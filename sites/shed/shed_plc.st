(* =====================================================================
   SATURN SITE CONTROLS - SHED PLC  (OpenPLC Runtime, Structured Text)
   Controls: 2 x piston air compressors (lead/lag), condensate drains, tank level, tank pump
   I/O:      Schneider TM3BCEIP coupler + TM3DI16 + TM3DQ16R + TM3AI4 (Modbus TCP)
   Brain:    OpenPLC on industrial DIN-rail PC.  Supervisor: Home Assistant.

   SAFETY: everything here is a PERMISSIVE. Each compressor's own pressure
   switch (high limit), overloads, Hand/Off/Auto switches and the tank
   low-level float cut-out are HARDWIRED and do not depend on this program.

   UNITS: pressure = bar x 100 (650 = 6.50 bar), tank level = % x 10 (255 = 25.5 %)
   ===================================================================== *)

(* ---------- helper: read one bit out of a 16-bit word ---------- *)
FUNCTION GETBIT : BOOL
  VAR_INPUT
    W : WORD;
    N : INT;
  END_VAR
  GETBIT := (SHR(W, N) AND WORD#16#0001) = WORD#16#0001;
END_FUNCTION

(* ---------- helper: scale a 4-20 mA raw value ---------- *)
FUNCTION SCALE420 : INT
  VAR_INPUT
    RAW      : INT;   (* raw value from analog module *)
    RAW4     : INT;   (* raw value at 4 mA *)
    RAW20    : INT;   (* raw value at 20 mA *)
    FULL     : INT;   (* engineering value at 20 mA *)
  END_VAR
  VAR
    span : DINT;
    v    : DINT;
  END_VAR
  span := INT_TO_DINT(RAW20) - INT_TO_DINT(RAW4);
  IF span <= 0 THEN
    span := 10000;
  END_IF;
  v := (INT_TO_DINT(RAW) - INT_TO_DINT(RAW4)) * INT_TO_DINT(FULL) / span;
  IF v < 0 THEN v := 0; END_IF;
  IF v > 32000 THEN v := 32000; END_IF;
  SCALE420 := DINT_TO_INT(v);
END_FUNCTION

(* ---------- helper: is a 4-20 mA raw value outside its valid band ---------- *)
FUNCTION AIBAD : BOOL
  VAR_INPUT
    RAW   : INT;
    RAW4  : INT;
    RAW20 : INT;
  END_VAR
  VAR
    margin : INT;
  END_VAR
  margin := (RAW20 - RAW4) / 20;   (* about 0.8 mA *)
  AIBAD := (RAW < RAW4 - margin) OR (RAW > RAW20 + margin);
END_FUNCTION

(* =====================================================================
   FB_COMPRESSOR - one piston compressor with pressure control,
   anti short-cycle, starts-per-hour limit, max-run / forced rest,
   fault handling and condensate drain.
   ===================================================================== *)
FUNCTION_BLOCK FB_COMPRESSOR
  VAR_INPUT
    SEC          : BOOL;  (* 1-second pulse, one scan long *)
    ENABLE       : BOOL;  (* from HA schedule (or comms-loss fallback) *)
    AUTO         : BOOL;  (* Hand/Off/Auto switch is in Auto *)
    RUN_FB       : BOOL;  (* contactor aux - compressor actually running *)
    HEALTHY      : BOOL;  (* overload / thermal healthy (24 V present = OK) *)
    PRESS        : INT;   (* bar x 100 *)
    PRESS_BAD    : BOOL;  (* pressure transmitter out of range *)
    CUT_IN       : INT;
    CUT_OUT      : INT;
    MIN_OFF_S    : INT;
    MAX_RUN_MIN  : INT;   (* minutes; set high (e.g. 999) to effectively disable *)
    REST_MIN     : INT;
    MAX_STARTS   : INT;   (* per hour *)
    DRAIN_INT_MIN: INT;
    DRAIN_OPEN_S : INT;
    SENSOR_MODE  : INT;   (* on transmitter fault: 0 = stop, 1 = run on compressor's own pressure switch *)
    FAULT_RESET  : BOOL;  (* one-scan pulse *)
    MAN_DRAIN    : BOOL;  (* one-scan pulse *)
    START_INHIBIT: BOOL;  (* staggered start: the other compressor started recently *)
  END_VAR
  VAR_OUTPUT
    RUN_OUT      : BOOL;
    DRAIN_OUT    : BOOL;
    STATE        : INT;   (* see register map *)
    FAULT        : INT;   (* 0 none, 1 overload, 2 fail to start, 3 transmitter, 4 pressure not rising *)
    STARTS_HR    : INT;
    START_PULSE  : BOOL;
    DRAIN_PULSE  : BOOL;
    AVAILABLE    : BOOL;  (* able to take load: Auto, no fault, not resting, under starts limit *)
  END_VAR
  VAR
    demand       : BOOL;
    run_cmd      : BOOL;
    permit       : BOOL;
    use_switch   : BOOL;
    resting      : BOOL;
    drain_req    : BOOL;
    drain_active : BOOL;
    prev_run_fb  : BOOL;
    off_secs     : INT := 0;   (* starts at 0 so a restart after power-up waits MIN_OFF_S *)
    run_secs     : INT := 0;
    rest_secs    : INT := 0;
    hr_secs      : INT := 0;
    fts_secs     : INT := 0;
    nr_secs      : INT := 0;
    drain_timer  : INT := 0;
    drain_secs   : INT := 0;
  END_VAR

  START_PULSE := FALSE;
  DRAIN_PULSE := FALSE;

  (* ---- fault reset ---- *)
  IF FAULT_RESET THEN
    FAULT := 0;
    resting := FALSE;
    fts_secs := 0;
    nr_secs := 0;
  END_IF;

  (* ---- one-hour window for the starts counter ---- *)
  IF SEC THEN
    hr_secs := hr_secs + 1;
    IF hr_secs >= 3600 THEN
      hr_secs := 0;
      STARTS_HR := 0;
    END_IF;
  END_IF;

  (* ---- running / stopped timers (based on ACTUAL running) ---- *)
  IF SEC THEN
    IF RUN_FB THEN
      off_secs := 0;
      IF run_secs < 32000 THEN run_secs := run_secs + 1; END_IF;
    ELSE
      run_secs := 0;
      IF off_secs < 32000 THEN off_secs := off_secs + 1; END_IF;
    END_IF;
  END_IF;

  (* ---- faults that latch ---- *)
  IF NOT HEALTHY THEN
    FAULT := 1;                                   (* overload / thermal trip *)
  END_IF;
  use_switch := PRESS_BAD AND (SENSOR_MODE = 1);
  IF PRESS_BAD AND (SENSOR_MODE = 0) AND (FAULT = 0) THEN
    FAULT := 3;                                   (* transmitter fault, stop *)
  END_IF;

  (* ---- pressure demand with hysteresis ---- *)
  IF use_switch THEN
    demand := TRUE;                               (* compressor's own switch does the cycling *)
  ELSIF PRESS <= CUT_IN THEN
    demand := TRUE;
  ELSIF PRESS >= CUT_OUT THEN
    demand := FALSE;
  END_IF;

  permit := ENABLE AND AUTO AND (FAULT = 0);

  (* ---- forced rest timer ---- *)
  IF resting AND SEC THEN
    rest_secs := rest_secs + 1;
    IF rest_secs >= REST_MIN * 60 THEN
      resting := FALSE;
    END_IF;
  END_IF;

  (* ---- start / stop ---- *)
  IF run_cmd THEN
    IF (NOT permit) OR (NOT demand) THEN
      run_cmd := FALSE;
    ELSIF (MAX_RUN_MIN > 0) AND (run_secs >= MAX_RUN_MIN * 60) THEN
      run_cmd := FALSE;                           (* duty-cycle protection *)
      resting := TRUE;
      rest_secs := 0;
    END_IF;
  ELSE
    IF permit AND demand AND (NOT resting) AND (NOT START_INHIBIT)
       AND (off_secs >= MIN_OFF_S) AND (STARTS_HR < MAX_STARTS) THEN
      run_cmd := TRUE;
      STARTS_HR := STARTS_HR + 1;
      START_PULSE := TRUE;
    END_IF;
  END_IF;

  (* ---- fail to start: commanded, below cut-out (so the compressor's
         own switch must be closed) but no run feedback for 10 s ---- *)
  IF run_cmd AND (NOT RUN_FB) AND (NOT use_switch) AND (PRESS < CUT_OUT) THEN
    IF SEC THEN fts_secs := fts_secs + 1; END_IF;
    IF fts_secs >= 10 THEN
      FAULT := 2;
      run_cmd := FALSE;
    END_IF;
  ELSE
    fts_secs := 0;
  END_IF;

  (* ---- pressure not rising: running 120 s and still under 0.5 bar ---- *)
  IF run_cmd AND RUN_FB AND (NOT use_switch) AND (PRESS < 50) THEN
    IF SEC THEN nr_secs := nr_secs + 1; END_IF;
    IF nr_secs >= 120 THEN
      FAULT := 4;
      run_cmd := FALSE;
    END_IF;
  ELSE
    nr_secs := 0;
  END_IF;

  IF FAULT <> 0 THEN
    run_cmd := FALSE;
  END_IF;

  RUN_OUT := run_cmd AND permit;
  AVAILABLE := AUTO AND (FAULT = 0) AND (NOT resting) AND (STARTS_HR < MAX_STARTS);

  (* ---- condensate drain ---- *)
  IF RUN_FB THEN
    IF SEC THEN drain_timer := drain_timer + 1; END_IF;
    IF drain_timer >= DRAIN_INT_MIN * 60 THEN
      drain_timer := 0;
      drain_req := TRUE;                          (* periodic drain while running *)
    END_IF;
  END_IF;
  IF prev_run_fb AND (NOT RUN_FB) THEN
    drain_req := TRUE;                            (* drain once at every stop *)
    drain_timer := 0;
  END_IF;
  prev_run_fb := RUN_FB;
  IF MAN_DRAIN THEN
    drain_req := TRUE;
  END_IF;

  IF drain_req AND (NOT drain_active) THEN
    drain_req := FALSE;
    drain_active := TRUE;
    drain_secs := 0;
    DRAIN_PULSE := TRUE;
  END_IF;
  IF drain_active AND SEC THEN
    drain_secs := drain_secs + 1;
  END_IF;
  IF drain_active AND (drain_secs >= DRAIN_OPEN_S) THEN
    drain_active := FALSE;
  END_IF;
  DRAIN_OUT := drain_active;

  (* ---- state for the screens ---- *)
  IF FAULT <> 0 THEN
    STATE := 6;                                   (* fault *)
  ELSIF NOT AUTO THEN
    STATE := 7;                                   (* HOA not in Auto *)
  ELSIF use_switch THEN
    STATE := 8;                                   (* running on own pressure switch *)
  ELSIF RUN_FB THEN
    STATE := 2;                                   (* running *)
  ELSIF NOT ENABLE THEN
    STATE := 0;                                   (* off by schedule *)
  ELSIF resting THEN
    STATE := 4;                                   (* duty-cycle rest *)
  ELSIF demand AND (STARTS_HR >= MAX_STARTS) THEN
    STATE := 5;                                   (* starts-per-hour limit *)
  ELSIF demand AND ((off_secs < MIN_OFF_S) OR START_INHIBIT) THEN
    STATE := 3;                                   (* anti short-cycle / stagger wait *)
  ELSE
    STATE := 1;                                   (* enabled, pressure OK *)
  END_IF;
END_FUNCTION_BLOCK

(* =====================================================================
   MAIN PROGRAM - SHED  (2 compressors lead/lag on one air main)
   ===================================================================== *)
PROGRAM shed_main
  VAR
    (* ---------- Schneider TM3 I/O via the coupler (OpenPLC slave device) ----------
       CONFIRM AGAINST THE COUPLER'S REGISTER MAP AT COMMISSIONING *)
    DI_WORD  AT %IW100 : WORD;   (* TM3DI16 inputs 0-15 as one word *)
    AI_PRESS AT %IW101 : INT;    (* TM3AI4 ch0: air main / receiver pressure 4-20 mA *)
    AI_TANK  AT %IW102 : INT;    (* TM3AI4 ch1: tank level 4-20 mA *)
    DQ_WORD  AT %QW100 : WORD;   (* TM3DQ16R outputs 0-15 as one word *)

    (* ---------- HA -> PLC (holding registers 1024 + n) ---------- *)
    HB            AT %MW0  : INT;   (* heartbeat, changes every write *)
    CMD_CMP_EN    AT %MW1  : INT;   (* compressed air enabled (schedule) *)
    CMD_PUMP_EN   AT %MW2  : INT;
    SP_CUT_IN     AT %MW3  : INT;   (* lead cut-in, bar x100 *)
    SP_CUT_OUT    AT %MW4  : INT;   (* lead cut-out, bar x100 *)
    SP_MIN_OFF_S  AT %MW5  : INT;
    SP_MAX_RUN    AT %MW6  : INT;
    SP_REST       AT %MW7  : INT;
    SP_MAX_STARTS AT %MW8  : INT;
    SP_DRAIN_INT  AT %MW9  : INT;
    SP_DRAIN_OPEN AT %MW10 : INT;
    SP_FALLBACK   AT %MW11 : INT;
    SP_SENS_MODE  AT %MW12 : INT;
    SP_PT_RANGE   AT %MW13 : INT;
    SP_RAW4       AT %MW14 : INT;
    SP_RAW20      AT %MW15 : INT;
    SP_TANK_SCALE AT %MW16 : INT;
    SP_TANK_OFF   AT %MW17 : INT;
    SP_TANK_ON    AT %MW18 : INT;
    SP_PUMP_MAX   AT %MW19 : INT;
    CMD_AUX       AT %MW20 : WORD;  (* bits 8-15 drive DQ8-DQ15 directly *)
    SP_LAG_OFF    AT %MW21 : INT;   (* lag starts/stops this much below lead, bar x100 *)
    SP_STAGGER_S  AT %MW22 : INT;   (* min seconds between the two compressors starting *)
    SP_ROTATE_H   AT %MW23 : INT;   (* swap lead every N hours (auto mode) *)
    SP_LEAD_MODE  AT %MW24 : INT;   (* 0 = auto rotate, 1 = C1 always lead, 2 = C2 always lead *)
    CMD_RESET     AT %MW30 : INT;   (* write 1, PLC clears *)
    CMD_DRAIN     AT %MW31 : INT;   (* write 1, PLC clears - drains both *)
    CMD_SWAP      AT %MW32 : INT;   (* write 1, PLC clears - swap lead now *)

    (* ---------- PLC -> HA ---------- *)
    ST_PRESS      AT %MW40 : INT;
    ST_C1_STATE   AT %MW41 : INT;
    ST_C1_FAULT   AT %MW42 : INT;
    ST_C1_STARTS  AT %MW43 : INT;
    ST_COMMS_OK   AT %MW44 : INT;
    ST_DRAIN_ON   AT %MW45 : INT;   (* bit0 = C1 drain, bit1 = C2 drain *)
    ST_TANK_PCT   AT %MW46 : INT;
    ST_PUMP_STATE AT %MW47 : INT;
    ST_DI_WORD    AT %MW48 : WORD;
    ST_DQ_WORD    AT %MW49 : WORD;
    ST_PUMP_FAULT AT %MW50 : INT;
    ST_C2_STATE   AT %MW51 : INT;
    ST_C2_FAULT   AT %MW52 : INT;
    ST_C2_STARTS  AT %MW53 : INT;
    ST_LEAD       AT %MW54 : INT;   (* 1 or 2 *)
    ST_FORCE_ON   AT %MW55 : INT;   (* 1 = forces active *)
    ST_FORCE_LEFT AT %MW56 : INT;   (* seconds until forces auto-clear *)
    ST_DI_EFF     AT %MW57 : WORD;  (* inputs as the logic sees them (after forces) *)
    ST_DQ_LOGIC   AT %MW58 : WORD;  (* outputs the logic wants (before forces) *)

    (* ---------- I/O forcing for testing / commissioning (written by Saturn Command Center via HA) ----------
       Forces auto-clear FORCE_MIN minutes after the last change, and on every PLC restart.
       Hardwired protection (pressure switches, overloads, HOA, float cut-outs) still applies. *)
    F_DI_MASK     AT %MW60 : WORD;  (* 1 = input bit forced *)
    F_DI_VAL      AT %MW61 : WORD;  (* forced input value *)
    F_DQ_MASK     AT %MW62 : WORD;  (* 1 = output bit forced *)
    F_DQ_VAL      AT %MW63 : WORD;  (* forced output value *)
    F_AI_MASK     AT %MW64 : WORD;  (* bit0 = AI0 forced, bit1 = AI1 forced *)
    F_AI0_RAW     AT %MW65 : INT;   (* forced raw value for AI0 *)
    F_AI1_RAW     AT %MW66 : INT;   (* forced raw value for AI1 *)
    F_MINUTES     AT %MW67 : INT;   (* auto-clear time, default 60 *)
    ST_AI0_RAW    AT %MW70 : INT;   (* real raw AI0 *)
    ST_AI1_RAW    AT %MW71 : INT;   (* real raw AI1 *)
    CNT_C1_SECS   AT %MD0  : DINT;  (* holding 2048-2049 *)
    CNT_C1_START  AT %MD1  : DINT;  (* holding 2050-2051 *)
    CNT_PUMP_SECS AT %MD2  : DINT;  (* holding 2052-2053 *)
    CNT_DRAINS    AT %MD3  : DINT;  (* holding 2054-2055, both compressors *)
    CNT_C2_SECS   AT %MD4  : DINT;  (* holding 2056-2057 *)
    CNT_C2_START  AT %MD5  : DINT;  (* holding 2058-2059 *)

    (* ---------- internal ---------- *)
    tick         : TON;
    sec          : BOOL;
    c1           : FB_COMPRESSOR;
    c2           : FB_COMPRESSOR;
    lead         : INT := 1;
    rot_secs     : DINT := 0;
    stagger_secs : INT := 0;
    inhibit1     : BOOL;
    inhibit2     : BOOL;
    lead_in      : INT;
    lead_out     : INT;
    lag_in       : INT;
    lag_out      : INT;
    c1_in, c1_out, c2_in, c2_out : INT;
    hb_prev      : INT;
    hb_secs      : INT := 0;
    comms_ok     : BOOL;
    fb_secs      : INT := 0;
    cmp_enable   : BOOL;
    pump_cmd     : BOOL := TRUE;    (* water stays on if HA has never spoken *)
    press        : INT;
    press_bad    : BOOL;
    tank_pct     : INT;
    tank_bad     : BOOL;
    level_ok     : BOOL := TRUE;
    pump_fault   : BOOL;
    pump_secs    : INT := 0;
    pump_en      : BOOL;
    do_reset     : BOOL;
    do_drain     : BOOL;
    do_swap      : BOOL;
    out          : WORD;
    di           : WORD;            (* inputs after forces *)
    ai_p         : INT;             (* AI0 after forces *)
    ai_t         : INT;             (* AI1 after forces *)
    f_secs       : INT := 0;
    f_prev       : WORD;            (* change detector *)
    f_sig        : WORD;

    (* inputs, named *)
    C1_AUTO, C1_RUN_FB, C1_HEALTHY : BOOL;
    C2_AUTO, C2_RUN_FB, C2_HEALTHY : BOOL;
    PUMP_AUTO, PUMP_RUN_FB, TANK_FLOAT_OK, SHED_DOOR_CLOSED : BOOL;
  END_VAR

  (* ---------- 1 second pulse ---------- *)
  tick(IN := NOT tick.Q, PT := T#1s);
  sec := tick.Q;

  (* ---------- defaults if a setpoint has never been written ---------- *)
  IF SP_CUT_IN     <= 0 THEN SP_CUT_IN     := 650;   END_IF;
  IF SP_CUT_OUT    <= SP_CUT_IN THEN SP_CUT_OUT := SP_CUT_IN + 150; END_IF;
  IF SP_MIN_OFF_S  <= 0 THEN SP_MIN_OFF_S  := 60;    END_IF;
  IF SP_MAX_RUN    <= 0 THEN SP_MAX_RUN    := 20;    END_IF;
  IF SP_REST       <= 0 THEN SP_REST       := 10;    END_IF;
  IF SP_MAX_STARTS <= 0 THEN SP_MAX_STARTS := 10;    END_IF;
  IF SP_DRAIN_INT  <= 0 THEN SP_DRAIN_INT  := 30;    END_IF;
  IF SP_DRAIN_OPEN <= 0 THEN SP_DRAIN_OPEN := 5;     END_IF;
  IF SP_FALLBACK   <= 0 THEN SP_FALLBACK   := 240;   END_IF;
  IF SP_PT_RANGE   <= 0 THEN SP_PT_RANGE   := 1600;  END_IF;
  IF SP_RAW20      <= SP_RAW4 THEN SP_RAW4 := 0; SP_RAW20 := 10000; END_IF;
  IF SP_TANK_SCALE <= 0 THEN SP_TANK_SCALE := 1000;  END_IF;
  IF SP_TANK_OFF   <= 0 THEN SP_TANK_OFF   := 150;   END_IF;
  IF SP_TANK_ON    <= SP_TANK_OFF THEN SP_TANK_ON := SP_TANK_OFF + 100; END_IF;
  IF SP_PUMP_MAX   <= 0 THEN SP_PUMP_MAX   := 30;    END_IF;
  IF SP_LAG_OFF    <= 0 THEN SP_LAG_OFF    := 50;    END_IF;
  IF SP_STAGGER_S  <= 0 THEN SP_STAGGER_S  := 10;    END_IF;
  IF SP_ROTATE_H   <= 0 THEN SP_ROTATE_H   := 24;    END_IF;
  IF (SP_LEAD_MODE < 0) OR (SP_LEAD_MODE > 2) THEN SP_LEAD_MODE := 0; END_IF;

  (* ---------- forcing: auto-clear timer ---------- *)
  IF F_MINUTES <= 0 THEN F_MINUTES := 60; END_IF;
  f_sig := F_DI_MASK XOR F_DI_VAL XOR F_DQ_MASK XOR F_DQ_VAL XOR F_AI_MASK
           XOR INT_TO_WORD(F_AI0_RAW) XOR INT_TO_WORD(F_AI1_RAW);
  IF f_sig <> f_prev THEN
    f_prev := f_sig;
    f_secs := 0;                       (* any change restarts the clock *)
  END_IF;
  IF (F_DI_MASK <> WORD#0) OR (F_DQ_MASK <> WORD#0) OR (F_AI_MASK <> WORD#0) THEN
    IF sec AND (f_secs < 32000) THEN f_secs := f_secs + 1; END_IF;
    IF f_secs >= F_MINUTES * 60 THEN
      F_DI_MASK := WORD#0; F_DI_VAL := WORD#0;
      F_DQ_MASK := WORD#0; F_DQ_VAL := WORD#0;
      F_AI_MASK := WORD#0;
      f_secs := 0;
    END_IF;
    ST_FORCE_ON := 1;
    ST_FORCE_LEFT := F_MINUTES * 60 - f_secs;
  ELSE
    f_secs := 0;
    ST_FORCE_ON := 0;
    ST_FORCE_LEFT := 0;
  END_IF;

  (* inputs and analogs as the logic sees them *)
  di := (DI_WORD AND NOT F_DI_MASK) OR (F_DI_VAL AND F_DI_MASK);
  IF (F_AI_MASK AND WORD#16#0001) <> WORD#0 THEN ai_p := F_AI0_RAW; ELSE ai_p := AI_PRESS; END_IF;
  IF (F_AI_MASK AND WORD#16#0002) <> WORD#0 THEN ai_t := F_AI1_RAW; ELSE ai_t := AI_TANK; END_IF;

  (* ---------- unpack digital inputs (TM3DI16) ---------- *)
  C1_AUTO          := GETBIT(di, 0);
  C1_RUN_FB        := GETBIT(di, 1);
  C1_HEALTHY       := GETBIT(di, 2);
  C2_AUTO          := GETBIT(di, 3);
  C2_RUN_FB        := GETBIT(di, 4);
  C2_HEALTHY       := GETBIT(di, 5);
  PUMP_AUTO        := GETBIT(di, 6);
  PUMP_RUN_FB      := GETBIT(di, 7);
  TANK_FLOAT_OK    := GETBIT(di, 8);
  SHED_DOOR_CLOSED := GETBIT(di, 9);

  (* ---------- analog ---------- *)
  press     := SCALE420(ai_p, SP_RAW4, SP_RAW20, SP_PT_RANGE);
  press_bad := AIBAD(ai_p, SP_RAW4, SP_RAW20);
  tank_pct  := SCALE420(ai_t, SP_RAW4, SP_RAW20, SP_TANK_SCALE);
  tank_bad  := AIBAD(ai_t, SP_RAW4, SP_RAW20);

  (* ---------- HA heartbeat / comms-loss fallback ---------- *)
  IF HB <> hb_prev THEN
    hb_prev := HB;
    hb_secs := 0;
  ELSIF sec AND (hb_secs < 32000) THEN
    hb_secs := hb_secs + 1;
  END_IF;
  comms_ok := hb_secs < 60;

  IF comms_ok THEN
    cmp_enable := CMD_CMP_EN <> 0;
    pump_cmd   := CMD_PUMP_EN <> 0;
    fb_secs    := 0;
  ELSE
    IF sec AND (fb_secs < 32000) THEN fb_secs := fb_secs + 1; END_IF;
    IF fb_secs >= SP_FALLBACK * 60 THEN
      cmp_enable := FALSE;
    END_IF;
  END_IF;

  do_reset := CMD_RESET <> 0;
  do_drain := CMD_DRAIN <> 0;
  do_swap  := CMD_SWAP <> 0;

  (* ---------- lead selection / rotation ---------- *)
  IF SP_LEAD_MODE = 1 THEN
    lead := 1;
    rot_secs := 0;
  ELSIF SP_LEAD_MODE = 2 THEN
    lead := 2;
    rot_secs := 0;
  ELSE
    IF sec THEN rot_secs := rot_secs + 1; END_IF;
    IF do_swap OR (rot_secs >= INT_TO_DINT(SP_ROTATE_H) * 3600) THEN
      IF lead = 1 THEN lead := 2; ELSE lead := 1; END_IF;
      rot_secs := 0;
    END_IF;
  END_IF;

  (* ---------- setpoints by role. If the lead can't take load
         (fault, not in Auto, resting, starts limit) the other one
         gets the lead setpoints. Uses last scan's AVAILABLE. ---------- *)
  lead_in  := SP_CUT_IN;
  lead_out := SP_CUT_OUT;
  lag_in   := SP_CUT_IN - SP_LAG_OFF;
  lag_out  := SP_CUT_OUT - SP_LAG_OFF;
  IF lag_in < 50 THEN lag_in := 50; END_IF;
  IF lag_out <= lag_in THEN lag_out := lag_in + 50; END_IF;

  IF lead = 1 THEN
    c1_in := lead_in; c1_out := lead_out;
    IF c1.AVAILABLE THEN
      c2_in := lag_in;  c2_out := lag_out;
    ELSE
      c2_in := lead_in; c2_out := lead_out;
    END_IF;
  ELSE
    c2_in := lead_in; c2_out := lead_out;
    IF c2.AVAILABLE THEN
      c1_in := lag_in;  c1_out := lag_out;
    ELSE
      c1_in := lead_in; c1_out := lead_out;
    END_IF;
  END_IF;

  (* ---------- staggered starts: never start both within SP_STAGGER_S ---------- *)
  IF c1.START_PULSE OR c2.START_PULSE THEN
    stagger_secs := 0;
  ELSIF sec AND (stagger_secs < 32000) THEN
    stagger_secs := stagger_secs + 1;
  END_IF;
  inhibit1 := stagger_secs < SP_STAGGER_S;
  inhibit2 := inhibit1;
  (* if both want to start in the same scan, the lead goes first *)
  IF (NOT inhibit1) AND (lead = 1) THEN inhibit2 := TRUE; END_IF;
  IF (NOT inhibit1) AND (lead = 2) THEN inhibit1 := TRUE; END_IF;
  (* ...but the lag must not wait forever if the lead isn't going to start *)
  IF (lead = 1) AND (NOT c1.AVAILABLE) THEN inhibit2 := stagger_secs < SP_STAGGER_S; END_IF;
  IF (lead = 2) AND (NOT c2.AVAILABLE) THEN inhibit1 := stagger_secs < SP_STAGGER_S; END_IF;
  IF (lead = 1) AND C1_RUN_FB THEN inhibit2 := stagger_secs < SP_STAGGER_S; END_IF;
  IF (lead = 2) AND C2_RUN_FB THEN inhibit1 := stagger_secs < SP_STAGGER_S; END_IF;

  (* ---------- compressors ---------- *)
  c1(SEC := sec, ENABLE := cmp_enable, AUTO := C1_AUTO, RUN_FB := C1_RUN_FB,
     HEALTHY := C1_HEALTHY, PRESS := press, PRESS_BAD := press_bad,
     CUT_IN := c1_in, CUT_OUT := c1_out, MIN_OFF_S := SP_MIN_OFF_S,
     MAX_RUN_MIN := SP_MAX_RUN, REST_MIN := SP_REST, MAX_STARTS := SP_MAX_STARTS,
     DRAIN_INT_MIN := SP_DRAIN_INT, DRAIN_OPEN_S := SP_DRAIN_OPEN,
     SENSOR_MODE := SP_SENS_MODE, FAULT_RESET := do_reset, MAN_DRAIN := do_drain,
     START_INHIBIT := inhibit1);

  c2(SEC := sec, ENABLE := cmp_enable, AUTO := C2_AUTO, RUN_FB := C2_RUN_FB,
     HEALTHY := C2_HEALTHY, PRESS := press, PRESS_BAD := press_bad,
     CUT_IN := c2_in, CUT_OUT := c2_out, MIN_OFF_S := SP_MIN_OFF_S,
     MAX_RUN_MIN := SP_MAX_RUN, REST_MIN := SP_REST, MAX_STARTS := SP_MAX_STARTS,
     DRAIN_INT_MIN := SP_DRAIN_INT, DRAIN_OPEN_S := SP_DRAIN_OPEN,
     SENSOR_MODE := SP_SENS_MODE, FAULT_RESET := do_reset, MAN_DRAIN := do_drain,
     START_INHIBIT := inhibit2);

  (* ---------- tank pump (a permissive: the pump's own pressure switch runs it) ---------- *)
  IF do_reset THEN
    pump_fault := FALSE;
    pump_secs := 0;
  END_IF;

  IF tank_bad THEN
    level_ok := TRUE;                 (* level sensor failed: rely on the float *)
  ELSIF tank_pct <= SP_TANK_OFF THEN
    level_ok := FALSE;
  ELSIF tank_pct >= SP_TANK_ON THEN
    level_ok := TRUE;
  END_IF;

  IF PUMP_RUN_FB THEN
    IF sec AND (pump_secs < 32000) THEN pump_secs := pump_secs + 1; END_IF;
    IF (SP_PUMP_MAX > 0) AND (pump_secs >= SP_PUMP_MAX * 60) THEN
      pump_fault := TRUE;             (* ran too long: burst pipe / leak / lost prime *)
    END_IF;
  ELSE
    pump_secs := 0;
  END_IF;

  pump_en := pump_cmd AND PUMP_AUTO AND TANK_FLOAT_OK AND level_ok AND (NOT pump_fault);

  (* ---------- pack outputs (TM3DQ16R) ---------- *)
  out := WORD#16#0000;
  IF c1.RUN_OUT   THEN out := out OR WORD#16#0001; END_IF;   (* DQ0 compressor 1 run *)
  IF c1.DRAIN_OUT THEN out := out OR WORD#16#0002; END_IF;   (* DQ1 compressor 1 drain *)
  IF c2.RUN_OUT   THEN out := out OR WORD#16#0004; END_IF;   (* DQ2 compressor 2 run *)
  IF c2.DRAIN_OUT THEN out := out OR WORD#16#0008; END_IF;   (* DQ3 compressor 2 drain *)
  IF pump_en      THEN out := out OR WORD#16#0010; END_IF;   (* DQ4 tank pump enable *)
  out := out OR (CMD_AUX AND WORD#16#FF00);                   (* DQ8-15 from HA *)
  ST_DQ_LOGIC := out;
  DQ_WORD := (out AND NOT F_DQ_MASK) OR (F_DQ_VAL AND F_DQ_MASK);   (* forced outputs override logic *)

  (* ---------- status to HA ---------- *)
  ST_PRESS     := press;
  ST_C1_STATE  := c1.STATE;
  ST_C1_FAULT  := c1.FAULT;
  ST_C1_STARTS := c1.STARTS_HR;
  ST_C2_STATE  := c2.STATE;
  ST_C2_FAULT  := c2.FAULT;
  ST_C2_STARTS := c2.STARTS_HR;
  ST_LEAD      := lead;
  IF comms_ok THEN ST_COMMS_OK := 1; ELSE ST_COMMS_OK := 0; END_IF;
  ST_DRAIN_ON := 0;
  IF c1.DRAIN_OUT THEN ST_DRAIN_ON := ST_DRAIN_ON + 1; END_IF;
  IF c2.DRAIN_OUT THEN ST_DRAIN_ON := ST_DRAIN_ON + 2; END_IF;
  ST_TANK_PCT  := tank_pct;
  IF pump_fault THEN
    ST_PUMP_STATE := 4;
  ELSIF NOT PUMP_AUTO THEN
    ST_PUMP_STATE := 5;
  ELSIF (NOT TANK_FLOAT_OK) OR (NOT level_ok) THEN
    ST_PUMP_STATE := 3;
  ELSIF PUMP_RUN_FB THEN
    ST_PUMP_STATE := 2;
  ELSIF pump_en THEN
    ST_PUMP_STATE := 1;
  ELSE
    ST_PUMP_STATE := 0;
  END_IF;
  IF pump_fault THEN ST_PUMP_FAULT := 1; ELSE ST_PUMP_FAULT := 0; END_IF;
  IF tank_bad THEN ST_PUMP_FAULT := ST_PUMP_FAULT + 2; END_IF;   (* +2 = tank sensor fault *)
  ST_DI_WORD := DI_WORD;
  ST_DQ_WORD := DQ_WORD;
  ST_DI_EFF  := di;
  ST_AI0_RAW := AI_PRESS;
  ST_AI1_RAW := AI_TANK;

  (* ---------- counters ---------- *)
  IF sec AND C1_RUN_FB   THEN CNT_C1_SECS   := CNT_C1_SECS + 1;   END_IF;
  IF sec AND C2_RUN_FB   THEN CNT_C2_SECS   := CNT_C2_SECS + 1;   END_IF;
  IF sec AND PUMP_RUN_FB THEN CNT_PUMP_SECS := CNT_PUMP_SECS + 1; END_IF;
  IF c1.START_PULSE THEN CNT_C1_START := CNT_C1_START + 1; END_IF;
  IF c2.START_PULSE THEN CNT_C2_START := CNT_C2_START + 1; END_IF;
  IF c1.DRAIN_PULSE THEN CNT_DRAINS := CNT_DRAINS + 1; END_IF;
  IF c2.DRAIN_PULSE THEN CNT_DRAINS := CNT_DRAINS + 1; END_IF;

  (* ---------- clear one-shot commands ---------- *)
  CMD_RESET := 0;
  CMD_DRAIN := 0;
  CMD_SWAP  := 0;
END_PROGRAM

CONFIGURATION Config0
  RESOURCE Res0 ON PLC
    TASK task0(INTERVAL := T#20ms, PRIORITY := 0);
    PROGRAM instance0 WITH task0 : shed_main;
  END_RESOURCE
END_CONFIGURATION
