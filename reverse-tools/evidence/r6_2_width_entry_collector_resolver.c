Target: T-Linter.dll; SHA256: 60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0

===== 0x1805fd880 1805fd880 FUN_1805fd880 =====

/* WARNING: Function: __security_check_cookie replaced with injection: security_check_cookie */
/* WARNING: Removing unreachable block (ram,0x0001805fe612) */
/* WARNING: Removing unreachable block (ram,0x0001805fe5cc) */
/* WARNING: Removing unreachable block (ram,0x0001805fe586) */
/* WARNING: Removing unreachable block (ram,0x0001805fe540) */
/* WARNING: Removing unreachable block (ram,0x0001805fe563) */
/* WARNING: Removing unreachable block (ram,0x0001805fe5a9) */
/* WARNING: Removing unreachable block (ram,0x0001805fe5ef) */
/* WARNING: Removing unreachable block (ram,0x0001805fe4fa) */
/* WARNING: Removing unreachable block (ram,0x0001805fe51d) */

void FUN_1805fd880(longlong param_1)

{
  char cVar1;
  int iVar2;
  undefined8 *puVar3;
  longlong lVar4;
  undefined8 *puVar5;
  void *pvVar6;
  ulonglong uVar7;
  undefined8 uVar8;
  undefined8 uVar9;
  undefined8 *puVar10;
  int iVar11;
  longlong *_Memory;
  void *pvVar12;
  longlong lVar13;
  longlong lVar14;
  longlong lVar15;
  uint uVar16;
  undefined8 *puVar17;
  undefined1 auStack_398 [32];
  uint local_378;
  int local_374;
  undefined8 local_370;
  undefined4 local_368;
  undefined1 *local_360;
  longlong local_358;
  longlong local_350;
  undefined8 local_348;
  undefined1 local_340;
  undefined7 uStack_33f;
  undefined8 local_330;
  ulonglong local_328;
  undefined1 local_320;
  undefined7 uStack_31f;
  undefined8 local_310;
  ulonglong local_308;
  undefined1 local_300;
  undefined7 uStack_2ff;
  undefined8 local_2f0;
  ulonglong local_2e8;
  undefined1 local_2e0;
  undefined7 uStack_2df;
  undefined8 local_2d0;
  ulonglong local_2c8;
  undefined1 local_2c0;
  undefined7 uStack_2bf;
  undefined8 local_2b0;
  ulonglong local_2a8;
  byte local_2a0;
  undefined7 uStack_29f;
  undefined8 local_290;
  ulonglong local_288;
  longlong *local_280;
  undefined8 uStack_278;
  longlong local_270;
  undefined1 local_268;
  undefined7 uStack_267;
  undefined8 local_258;
  ulonglong local_250;
  byte local_248;
  undefined7 uStack_247;
  undefined8 local_238;
  ulonglong local_230;
  byte local_228;
  undefined7 uStack_227;
  undefined8 local_218;
  ulonglong local_210;
  byte local_208;
  undefined7 uStack_207;
  undefined8 local_1f8;
  ulonglong local_1f0;
  byte local_1e8;
  undefined7 uStack_1e7;
  undefined8 local_1d8;
  ulonglong local_1d0;
  byte local_1c8;
  undefined7 uStack_1c7;
  undefined8 local_1b8;
  ulonglong local_1b0;
  byte local_1a8;
  undefined7 uStack_1a7;
  undefined8 local_198;
  ulonglong local_190;
  byte local_188;
  undefined7 uStack_187;
  undefined8 local_178;
  ulonglong local_170;
  byte local_168;
  undefined7 uStack_167;
  undefined8 local_158;
  ulonglong local_150;
  undefined1 local_148;
  undefined7 uStack_147;
  undefined8 local_138;
  ulonglong local_130;
  undefined1 local_128;
  undefined7 uStack_127;
  undefined8 local_118;
  ulonglong local_110;
  undefined1 local_108;
  undefined7 uStack_107;
  undefined8 local_f8;
  ulonglong local_f0;
  undefined1 local_e8 [16];
  undefined8 local_d8;
  undefined8 local_d0;
  undefined1 local_c8 [16];
  undefined8 local_b8;
  undefined8 local_b0;
  undefined1 local_a8 [16];
  undefined8 local_98;
  undefined8 local_90;
  undefined4 local_88;
  undefined1 local_80 [16];
  undefined8 local_70;
  undefined8 local_68;
  undefined1 local_60 [16];
  undefined8 local_50;
  undefined8 local_48;
  ulonglong local_38;
  
  local_348 = 0xfffffffffffffffe;
  local_38 = DAT_18131ef10 ^ (ulonglong)auStack_398;
  uVar16 = 0;
  local_378 = 0;
  puVar3 = *(undefined8 **)(param_1 + 0x88);
  puVar5 = (undefined8 *)*puVar3;
  local_358 = param_1;
  do {
    do {
      while( true ) {
        do {
          puVar17 = puVar5;
          if (puVar17 == puVar3) {
            return;
          }
          local_288 = 0xf;
          local_290 = 0;
          local_2a0 = 0;
          FUN_180040550(&local_2a0,puVar17 + 4,0,0xffffffffffffffff);
          FUN_1801acf00(&local_280,puVar17 + 8);
          lVar15 = *local_280;
          local_350 = lVar15;
          if (lVar15 == 0) {
            if (local_280 != (longlong *)0x0) {
              uVar7 = local_270 - (longlong)local_280 >> 3;
              if (0x1fffffffffffffff < uVar7) {
                    /* WARNING: Subroutine does not return */
                _invalid_parameter_noinfo_noreturn();
              }
              _Memory = local_280;
              if (0xfff < uVar7 * 8) {
                if (((ulonglong)local_280 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
                  _invalid_parameter_noinfo_noreturn();
                }
                _Memory = (longlong *)local_280[-1];
                if (local_280 <= _Memory) {
                    /* WARNING: Subroutine does not return */
                  _invalid_parameter_noinfo_noreturn();
                }
                if ((ulonglong)((longlong)local_280 - (longlong)_Memory) < 8) {
                    /* WARNING: Subroutine does not return */
                  _invalid_parameter_noinfo_noreturn();
                }
                if (0x27 < (ulonglong)((longlong)local_280 - (longlong)_Memory)) {
                    /* WARNING: Subroutine does not return */
                  _invalid_parameter_noinfo_noreturn();
                }
              }
              free(_Memory);
              local_280 = (longlong *)0x0;
              uStack_278 = 0;
              local_270 = 0;
            }
            if (0xf < local_288) {
              pvVar6 = (void *)CONCAT71(uStack_29f,local_2a0);
              pvVar12 = pvVar6;
              if (0xfff < local_288 + 1) {
                if ((local_2a0 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
                  _invalid_parameter_noinfo_noreturn();
                }
                pvVar12 = *(void **)((longlong)pvVar6 - 8);
                if (pvVar6 <= pvVar12) {
                    /* WARNING: Subroutine does not return */
                  _invalid_parameter_noinfo_noreturn();
                }
                if ((ulonglong)((longlong)pvVar6 - (longlong)pvVar12) < 8) {
                    /* WARNING: Subroutine does not return */
                  _invalid_parameter_noinfo_noreturn();
                }
                if (0x27 < (ulonglong)((longlong)pvVar6 - (longlong)pvVar12)) {
                    /* WARNING: Subroutine does not return */
                  _invalid_parameter_noinfo_noreturn();
                }
              }
              free(pvVar12);
            }
          }
          else {
            FUN_1805fefc0(param_1);
            FUN_180600050(param_1,lVar15);
            lVar14 = DAT_1813591d0;
            for (lVar13 = DAT_1813591c8; lVar13 != lVar14; lVar13 = lVar13 + 0x68) {
              FUN_1805fcf00(lVar13);
            }
            DAT_1813591d0 = DAT_1813591c8;
            FUN_1805fe650(param_1,lVar15);
            lVar13 = DAT_1813591d0;
            for (lVar15 = DAT_1813591c8; lVar15 != lVar13; lVar15 = lVar15 + 0x68) {
              iVar11 = *(int *)(lVar15 + 0x48);
              lVar4 = *(longlong *)(lVar15 + 0x58);
              local_374 = iVar11;
              for (lVar14 = *(longlong *)(lVar15 + 0x50); lVar14 != lVar4; lVar14 = lVar14 + 0x28) {
                iVar2 = *(int *)(lVar14 + 0x20);
                uVar7 = local_370 >> 0x20;
                local_370 = CONCAT44((int)uVar7,iVar2);
                if (((iVar11 != -1) && (iVar2 != -1)) && (iVar11 != iVar2)) {
                  local_2a8 = 0xf;
                  local_2b0 = 0;
                  local_2c0 = 0;
                  FUN_180040550(&local_2c0,local_358 + 0x10,0,0xffffffffffffffff);
                  local_2c8 = 0xf;
                  local_2d0 = 0;
                  local_2e0 = 0;
                  FUN_180040550(&local_2e0,local_350 + 0x1e0,0);
                  local_368 = *(undefined4 *)(lVar15 + 0x24);
                  if ((*(longlong *)(lVar15 + 0x38) == 0) || (*(longlong *)(lVar14 + 0x10) == 0)) {
                    local_328 = 0xf;
                    local_330 = 0;
                    local_340 = 0;
                    FUN_180040680(&local_340,&DAT_180de47b9,0);
                    local_360 = &local_340;
                    local_378 = uVar16 | 8;
                  }
                  else {
                    uVar8 = FUN_18003e420(&local_108,lVar15 + 0x28,&DAT_180eec528);
                    local_378 = uVar16 | 1;
                    uVar8 = FUN_1801a4ac0(&local_128,uVar8,lVar14);
                    local_378 = uVar16 | 3;
                    local_360 = (undefined1 *)FUN_1801380f0(&local_148,uVar8,&DAT_180f01628);
                    local_378 = uVar16 | 7;
                  }
                  local_370 = FUN_180133060(&local_168,local_370 & 0xffffffff);
                  uVar8 = FUN_180133060(&local_188,local_374);
                  uVar9 = FUN_18003e420(&local_1a8,lVar15,&DAT_180f01638);
                  uVar9 = FUN_1800d36a0(&local_1c8,uVar9,local_360);
                  uVar9 = FUN_1801380f0(&local_1e8,uVar9,&DAT_180f01618);
                  uVar8 = FUN_1800d36a0(&local_208,uVar9,uVar8);
                  uVar8 = FUN_1801380f0(&local_228,uVar8,&DAT_180f003f4);
                  uVar8 = FUN_1801380f0(&local_248,uVar8,&DAT_180f01608);
                  FUN_1800d36a0(&local_268,uVar8,local_370);
                  if (0xf < local_230) {
                    pvVar6 = (void *)CONCAT71(uStack_247,local_248);
                    pvVar12 = pvVar6;
                    if (0xfff < local_230 + 1) {
                      if ((local_248 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      pvVar12 = *(void **)((longlong)pvVar6 - 8);
                      if (pvVar6 <= pvVar12) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if ((ulonglong)((longlong)pvVar6 - (longlong)pvVar12) < 8) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if (0x27 < (ulonglong)((longlong)pvVar6 - (longlong)pvVar12)) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                    }
                    free(pvVar12);
                  }
                  local_230 = 0xf;
                  local_238 = 0;
                  local_248 = 0;
                  if (0xf < local_210) {
                    pvVar6 = (void *)CONCAT71(uStack_227,local_228);
                    pvVar12 = pvVar6;
                    if (0xfff < local_210 + 1) {
                      if ((local_228 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      pvVar12 = *(void **)((longlong)pvVar6 - 8);
                      if (pvVar6 <= pvVar12) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if ((ulonglong)((longlong)pvVar6 - (longlong)pvVar12) < 8) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if (0x27 < (ulonglong)((longlong)pvVar6 - (longlong)pvVar12)) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                    }
                    free(pvVar12);
                  }
                  local_210 = 0xf;
                  local_218 = 0;
                  local_228 = 0;
                  if (0xf < local_1f0) {
                    pvVar6 = (void *)CONCAT71(uStack_207,local_208);
                    pvVar12 = pvVar6;
                    if (0xfff < local_1f0 + 1) {
                      if ((local_208 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      pvVar12 = *(void **)((longlong)pvVar6 - 8);
                      if (pvVar6 <= pvVar12) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if ((ulonglong)((longlong)pvVar6 - (longlong)pvVar12) < 8) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if (0x27 < (ulonglong)((longlong)pvVar6 - (longlong)pvVar12)) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                    }
                    free(pvVar12);
                  }
                  local_1f0 = 0xf;
                  local_1f8 = 0;
                  local_208 = 0;
                  if (0xf < local_1d0) {
                    pvVar6 = (void *)CONCAT71(uStack_1e7,local_1e8);
                    pvVar12 = pvVar6;
                    if (0xfff < local_1d0 + 1) {
                      if ((local_1e8 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      pvVar12 = *(void **)((longlong)pvVar6 - 8);
                      if (pvVar6 <= pvVar12) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if ((ulonglong)((longlong)pvVar6 - (longlong)pvVar12) < 8) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if (0x27 < (ulonglong)((longlong)pvVar6 - (longlong)pvVar12)) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                    }
                    free(pvVar12);
                  }
                  local_1d0 = 0xf;
                  local_1d8 = 0;
                  local_1e8 = 0;
                  if (0xf < local_1b0) {
                    pvVar6 = (void *)CONCAT71(uStack_1c7,local_1c8);
                    pvVar12 = pvVar6;
                    if (0xfff < local_1b0 + 1) {
                      if ((local_1c8 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      pvVar12 = *(void **)((longlong)pvVar6 - 8);
                      if (pvVar6 <= pvVar12) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if ((ulonglong)((longlong)pvVar6 - (longlong)pvVar12) < 8) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if (0x27 < (ulonglong)((longlong)pvVar6 - (longlong)pvVar12)) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                    }
                    free(pvVar12);
                  }
                  local_1b0 = 0xf;
                  local_1b8 = 0;
                  local_1c8 = 0;
                  if (0xf < local_190) {
                    pvVar6 = (void *)CONCAT71(uStack_1a7,local_1a8);
                    pvVar12 = pvVar6;
                    if (0xfff < local_190 + 1) {
                      if ((local_1a8 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      pvVar12 = *(void **)((longlong)pvVar6 - 8);
                      if (pvVar6 <= pvVar12) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if ((ulonglong)((longlong)pvVar6 - (longlong)pvVar12) < 8) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if (0x27 < (ulonglong)((longlong)pvVar6 - (longlong)pvVar12)) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                    }
                    free(pvVar12);
                  }
                  local_190 = 0xf;
                  local_198 = 0;
                  local_1a8 = 0;
                  if (0xf < local_170) {
                    pvVar6 = (void *)CONCAT71(uStack_187,local_188);
                    pvVar12 = pvVar6;
                    if (0xfff < local_170 + 1) {
                      if ((local_188 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      pvVar12 = *(void **)((longlong)pvVar6 - 8);
                      if (pvVar6 <= pvVar12) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if ((ulonglong)((longlong)pvVar6 - (longlong)pvVar12) < 8) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if (0x27 < (ulonglong)((longlong)pvVar6 - (longlong)pvVar12)) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                    }
                    free(pvVar12);
                  }
                  local_170 = 0xf;
                  local_178 = 0;
                  local_188 = 0;
                  if (0xf < local_150) {
                    pvVar6 = (void *)CONCAT71(uStack_167,local_168);
                    pvVar12 = pvVar6;
                    if (0xfff < local_150 + 1) {
                      if ((local_168 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      pvVar12 = *(void **)((longlong)pvVar6 - 8);
                      if (pvVar6 <= pvVar12) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if ((ulonglong)((longlong)pvVar6 - (longlong)pvVar12) < 8) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if (0x27 < (ulonglong)((longlong)pvVar6 - (longlong)pvVar12)) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                    }
                    free(pvVar12);
                  }
                  local_150 = 0xf;
                  local_158 = 0;
                  local_168 = 0;
                  uVar16 = local_378;
                  if ((local_378 & 8) != 0) {
                    uVar16 = local_378 & 0xfffffff7;
                    local_378 = uVar16;
                    if (0xf < local_328) {
                      FUN_1800409e0(&local_340,CONCAT71(uStack_33f,local_340),local_328 + 1);
                    }
                    local_328 = 0xf;
                    local_330 = 0;
                    local_340 = 0;
                  }
                  if ((uVar16 & 4) != 0) {
                    uVar16 = uVar16 & 0xfffffffb;
                    local_378 = uVar16;
                    if (0xf < local_130) {
                      FUN_1800409e0(&local_148,CONCAT71(uStack_147,local_148),local_130 + 1);
                    }
                    local_130 = 0xf;
                    local_138 = 0;
                    local_148 = 0;
                  }
                  if ((uVar16 & 2) != 0) {
                    uVar16 = uVar16 & 0xfffffffd;
                    local_378 = uVar16;
                    if (0xf < local_110) {
                      FUN_1800409e0(&local_128,CONCAT71(uStack_127,local_128),local_110 + 1);
                    }
                    local_110 = 0xf;
                    local_118 = 0;
                    local_128 = 0;
                  }
                  if ((uVar16 & 1) != 0) {
                    uVar16 = uVar16 & 0xfffffffe;
                    local_378 = uVar16;
                    if (0xf < local_f0) {
                      FUN_1800409e0(&local_108,CONCAT71(uStack_107,local_108),local_f0 + 1);
                    }
                    local_f0 = 0xf;
                    local_f8 = 0;
                    local_108 = 0;
                  }
                  local_2e8 = 0xf;
                  local_2f0 = 0;
                  local_300 = 0;
                  FUN_180040680(&local_300,"Warning",7);
                  local_308 = 0xf;
                  local_310 = 0;
                  local_320 = 0;
                  FUN_180040680(&local_320,&DAT_180de47b9,0);
                  local_d0 = 0xf;
                  local_d8 = 0;
                  local_e8[0] = 0;
                  FUN_180040550(local_e8,&local_2c0,0,0xffffffffffffffff);
                  local_b0 = 0xf;
                  local_b8 = 0;
                  local_c8[0] = 0;
                  FUN_180040550(local_c8,&local_268,0);
                  local_90 = 0xf;
                  local_98 = 0;
                  local_a8[0] = 0;
                  FUN_180040550(local_a8,&local_2e0,0);
                  local_88 = local_368;
                  local_68 = 0xf;
                  local_70 = 0;
                  local_80[0] = 0;
                  FUN_180040550(local_80,&local_300,0);
                  local_48 = 0xf;
                  local_50 = 0;
                  local_60[0] = 0;
                  FUN_180040550(local_60,&local_320,0);
                  FUN_18049cca0(&DAT_18136fd88,local_e8);
                  FUN_18049bf90(local_e8);
                  if (0xf < local_308) {
                    FUN_1800409e0(&local_320,CONCAT71(uStack_31f,local_320),local_308 + 1);
                  }
                  local_308 = 0xf;
                  local_310 = 0;
                  local_320 = 0;
                  if (0xf < local_2e8) {
                    FUN_1800409e0(&local_300,CONCAT71(uStack_2ff,local_300),local_2e8 + 1);
                  }
                  local_2e8 = 0xf;
                  local_2f0 = 0;
                  local_300 = 0;
                  if (0xf < local_250) {
                    FUN_1800409e0(&local_268,CONCAT71(uStack_267,local_268),local_250 + 1);
                  }
                  local_250 = 0xf;
                  local_258 = 0;
                  local_268 = 0;
                  if (0xf < local_2c8) {
                    FUN_1800409e0(&local_2e0,CONCAT71(uStack_2df,local_2e0),local_2c8 + 1);
                  }
                  local_2c8 = 0xf;
                  local_2d0 = 0;
                  local_2e0 = 0;
                  if (0xf < local_2a8) {
                    FUN_1800409e0(&local_2c0,CONCAT71(uStack_2bf,local_2c0));
                  }
                  local_2a8 = 0xf;
                  local_2b0 = 0;
                  local_2c0 = 0;
                  iVar11 = local_374;
                }
              }
            }
            FUN_18049bf10(&local_2a0);
            param_1 = local_358;
          }
          puVar5 = puVar17;
        } while (*(char *)((longlong)puVar17 + 0x19) != '\0');
        puVar5 = (undefined8 *)puVar17[2];
        if (*(char *)((longlong)puVar5 + 0x19) != '\0') break;
        cVar1 = *(char *)((longlong)*puVar5 + 0x19);
        puVar17 = (undefined8 *)*puVar5;
        while (cVar1 == '\0') {
          cVar1 = *(char *)((longlong)*puVar17 + 0x19);
          puVar5 = puVar17;
          puVar17 = (undefined8 *)*puVar17;
        }
      }
      puVar5 = (undefined8 *)puVar17[1];
    } while (*(char *)((longlong)puVar5 + 0x19) != '\0');
    do {
      puVar10 = puVar5;
      puVar5 = puVar10;
      if (puVar17 != (undefined8 *)puVar10[2]) break;
      puVar5 = (undefined8 *)puVar10[1];
      puVar17 = puVar10;
    } while (*(char *)((longlong)puVar5 + 0x19) == '\0');
  } while( true );
}



===== 0x1805fe650 1805fe650 FUN_1805fe650 =====

/* WARNING: Function: __security_check_cookie replaced with injection: security_check_cookie */

void FUN_1805fe650(undefined8 param_1,longlong param_2)

{
  longlong *plVar1;
  char cVar2;
  int iVar3;
  undefined8 uVar4;
  longlong lVar5;
  longlong lVar6;
  ulonglong uVar7;
  undefined1 auStack_128 [32];
  int local_108;
  int local_104;
  undefined1 *local_100;
  uint local_f8;
  undefined8 local_f0;
  int local_e8;
  int local_e4;
  longlong local_e0;
  undefined1 local_d8 [40];
  longlong local_b0;
  undefined8 local_a8;
  undefined1 local_98 [16];
  undefined8 local_88;
  undefined8 local_80;
  undefined4 local_78;
  undefined4 local_74;
  undefined1 local_70 [16];
  undefined8 local_60;
  undefined8 local_58;
  undefined4 local_50;
  undefined8 local_48;
  undefined8 uStack_40;
  undefined8 local_38;
  ulonglong local_28;
  
  if (param_2 != 0) {
    local_a8 = 0xfffffffffffffffe;
    local_28 = DAT_18131ef10 ^ (ulonglong)auStack_128;
    if (*(int *)(param_2 + 4) != 0x53) {
      uVar7 = 0;
      local_f0 = param_1;
      if ((*(int *)(param_2 + 4) == 0x56) &&
         ((*(longlong *)(param_2 + 0x10) - *(longlong *)(param_2 + 8) & 0xfffffffffffffff8U) == 0x10
         )) {
        plVar1 = *(longlong **)(param_2 + 8);
        local_e0 = plVar1[1];
        if (*(int *)(local_e0 + 4) - 0x3cU < 2) {
          local_b0 = *plVar1;
          local_e8 = -1;
          local_e4 = -1;
          local_f8 = local_f8 & 0xffffff00;
          if (*(int *)(local_b0 + 4) == 0x13) {
            local_100 = local_d8;
            uVar4 = FUN_1800d77b0(local_d8,*plVar1 + 0x48);
            cVar2 = FUN_1805fee80(param_1,uVar4);
            local_f8 = local_f8 & 0xff;
            if (cVar2 != '\0') {
              local_f8 = 1;
            }
          }
          FUN_1805ff100(param_1,local_e0,&local_e4,1);
          FUN_1805ff100(param_1,local_b0,&local_e8,0);
          lVar6 = local_e0;
          if (((local_e8 != -1) && (local_e8 == local_e4)) &&
             ((*(longlong *)(local_e0 + 0x10) - *(longlong *)(local_e0 + 8) & 0xfffffffffffffff8U)
              == 0x10)) {
            if (((char)local_f8 == '\0') &&
               (*(int *)(*(longlong *)(*(longlong *)(param_2 + 8) + 8) + 4) == 0x3d)) {
              return;
            }
            if (((*(int *)(**(longlong **)(param_2 + 8) + 4) == 0x13) &&
                (*(int *)(**(longlong **)(local_e0 + 8) + 4) == 0x13)) &&
               ((iVar3 = FUN_1800dc750(**(longlong **)(param_2 + 8) + 0x48,
                                       **(longlong **)(local_e0 + 8) + 0x48), iVar3 == 0 &&
                (lVar5 = *(longlong *)(*(longlong *)(lVar6 + 8) + 8), *(int *)(lVar5 + 4) == 0x10)))
               ) {
              if ((*(longlong *)(lVar5 + 0x90) - *(longlong *)(lVar5 + 0x88) == 1) &&
                 (**(byte **)(lVar5 + 0x88) < 2)) {
                return;
              }
              lVar5 = FUN_1808dafe0(lVar5,*(undefined1 *)(lVar5 + 0xa4));
              if (lVar5 == 0) {
                return;
              }
              lVar6 = *(longlong *)(*(longlong *)(lVar6 + 8) + 8);
              lVar6 = FUN_1808dafe0(lVar6,*(undefined1 *)(lVar6 + 0xa4));
              if (lVar6 == 1) {
                return;
              }
            }
            else if ((((*(int *)(**(longlong **)(param_2 + 8) + 4) == 0x13) &&
                      (*(int *)(**(longlong **)(lVar6 + 8) + 4) == 0x13)) &&
                     (lVar6 = (*(longlong **)(lVar6 + 8))[1], *(int *)(lVar6 + 4) == 0x10)) &&
                    ((*(longlong *)(lVar6 + 0x90) - *(longlong *)(lVar6 + 0x88) == 1 &&
                     (**(byte **)(lVar6 + 0x88) < 2)))) {
              return;
            }
          }
        }
        uVar4 = **(undefined8 **)(param_2 + 8);
        local_108 = -1;
        local_104 = -1;
        FUN_1805ff100(local_f0,(*(undefined8 **)(param_2 + 8))[1],&local_104,0);
        param_1 = local_f0;
        FUN_1805ff100(local_f0,uVar4,&local_108,0);
        if (((local_108 != local_104) && (local_108 != -1)) && (local_104 != -1)) {
          local_74 = *(undefined4 *)(param_2 + 0x214);
          local_78 = *(undefined4 *)(param_2 + 4);
          local_80 = 0xf;
          local_88 = 0;
          local_98[0] = 0;
          local_100 = local_70;
          local_58 = 0xf;
          local_60 = 0;
          local_70[0] = 0;
          local_50 = 0;
          local_48 = 0;
          uStack_40 = 0;
          local_38 = 0;
          FUN_180040680(local_98,&DAT_180f015d8,8);
          local_100 = local_d8;
          uVar4 = FUN_1800d77b0(local_d8,**(longlong **)(param_2 + 8) + 0x48);
          FUN_1805fd4f0(local_98,uVar4,local_108);
          local_100 = local_d8;
          uVar4 = FUN_18020e850(local_d8,*(longlong *)(*(longlong *)(param_2 + 8) + 8) + 0x48,
                                &local_104);
          FUN_1805fc480(local_98,uVar4);
          FUN_180600b70(&DAT_1813591c8,local_98);
          FUN_1805fcf00(local_98);
        }
      }
      if (((*(int *)(param_2 + 4) - 0x34U < 4) || (*(int *)(param_2 + 4) - 0x3aU < 2)) &&
         ((*(longlong *)(param_2 + 0x10) - *(longlong *)(param_2 + 8) & 0xfffffffffffffff8U) == 0x10
         )) {
        uVar4 = **(undefined8 **)(param_2 + 8);
        local_104 = -1;
        local_108 = -1;
        FUN_1805ff100(local_f0,(*(undefined8 **)(param_2 + 8))[1],&local_108,0);
        param_1 = local_f0;
        FUN_1805ff100(local_f0,uVar4,&local_104,0);
        if ((local_104 != -1) && (local_108 != -1)) {
          local_74 = *(undefined4 *)(param_2 + 0x214);
          local_78 = *(undefined4 *)(param_2 + 4);
          local_80 = 0xf;
          local_88 = 0;
          local_98[0] = 0;
          local_100 = local_70;
          local_58 = 0xf;
          local_60 = 0;
          local_70[0] = 0;
          local_50 = 0;
          local_48 = 0;
          uStack_40 = 0;
          local_38 = 0;
          FUN_180040680(local_98,&DAT_180f015e8,10);
          local_100 = local_d8;
          uVar4 = FUN_1800d77b0(local_d8,**(longlong **)(param_2 + 8) + 0x48);
          FUN_1805fd4f0(local_98,uVar4,local_104);
          local_100 = local_d8;
          uVar4 = FUN_18020e850(local_d8,*(longlong *)(*(longlong *)(param_2 + 8) + 8) + 0x48,
                                &local_108);
          FUN_1805fc480(local_98,uVar4);
          FUN_180600b70(&DAT_1813591c8,local_98);
          FUN_1805fcf00(local_98);
        }
      }
      if ((*(int *)(param_2 + 4) - 0x25U < 4) &&
         ((*(longlong *)(param_2 + 0x10) - *(longlong *)(param_2 + 8) & 0xfffffffffffffff8U) == 0x10
         )) {
        uVar4 = **(undefined8 **)(param_2 + 8);
        local_104 = -1;
        local_108 = -1;
        FUN_1805ff100(local_f0,(*(undefined8 **)(param_2 + 8))[1],&local_108,0);
        param_1 = local_f0;
        FUN_1805ff100(local_f0,uVar4,&local_104,0);
        if ((local_104 != -1) && (local_108 != -1)) {
          local_74 = *(undefined4 *)(param_2 + 0x214);
          local_78 = *(undefined4 *)(param_2 + 4);
          local_80 = 0xf;
          local_88 = 0;
          local_98[0] = 0;
          local_100 = local_70;
          local_58 = 0xf;
          local_60 = 0;
          local_70[0] = 0;
          local_50 = 0;
          local_48 = 0;
          uStack_40 = 0;
          local_38 = 0;
          FUN_180040680(local_98,&DAT_180f015f8,10);
          local_100 = local_d8;
          uVar4 = FUN_1800d77b0(local_d8,**(longlong **)(param_2 + 8) + 0x48);
          FUN_1805fd4f0(local_98,uVar4,local_104);
          local_100 = local_d8;
          uVar4 = FUN_18020e850(local_d8,*(longlong *)(*(longlong *)(param_2 + 8) + 8) + 0x48,
                                &local_108);
          FUN_1805fc480(local_98,uVar4);
          FUN_180600b70(&DAT_1813591c8,local_98);
          FUN_1805fcf00(local_98);
        }
      }
      if (*(longlong *)(param_2 + 0x10) - *(longlong *)(param_2 + 8) >> 3 != 0) {
        do {
          FUN_1805fe650(param_1,*(undefined8 *)(*(longlong *)(param_2 + 8) + uVar7 * 8));
          uVar7 = uVar7 + 1;
        } while (uVar7 < (ulonglong)
                         (*(longlong *)(param_2 + 0x10) - *(longlong *)(param_2 + 8) >> 3));
      }
    }
  }
  return;
}



===== 0x1805ff100 1805ff100 FUN_1805ff100 =====

/* WARNING: Function: __security_check_cookie replaced with injection: security_check_cookie */

void FUN_1805ff100(undefined1 *param_1,longlong param_2,uint *param_3,char param_4)

{
  undefined8 *puVar1;
  uint uVar2;
  int iVar3;
  longlong lVar4;
  undefined4 *puVar5;
  uint *puVar6;
  undefined1 *puVar7;
  undefined8 *puVar8;
  undefined8 uVar9;
  char *_Str;
  longlong lVar10;
  uint uVar11;
  undefined1 *puVar12;
  size_t sVar13;
  undefined1 *puVar14;
  double dVar15;
  undefined1 auStack_228 [32];
  char local_208 [4];
  uint local_204;
  undefined8 local_200;
  uint local_1f8;
  undefined1 *local_1f0;
  undefined1 *local_1e8;
  int local_1e0 [4];
  uint *local_1d0;
  undefined8 local_1c8;
  undefined1 local_1c0 [32];
  undefined1 local_1a0 [32];
  undefined1 local_180 [32];
  undefined1 local_160 [32];
  undefined1 local_140 [32];
  undefined1 local_120 [32];
  undefined1 local_100;
  undefined7 uStack_ff;
  ulonglong local_f0;
  ulonglong local_e8;
  undefined1 local_e0;
  undefined7 uStack_df;
  undefined8 local_d0;
  ulonglong local_c8;
  undefined1 local_c0;
  undefined8 local_b0;
  undefined8 local_a8;
  undefined1 local_a0 [32];
  undefined1 local_80;
  undefined7 uStack_7f;
  undefined8 local_70;
  ulonglong local_68;
  undefined1 local_60;
  undefined7 uStack_5f;
  undefined8 local_50;
  ulonglong local_48;
  undefined1 local_40 [32];
  ulonglong local_20;
  
  local_1c8 = 0xfffffffffffffffe;
  local_20 = DAT_18131ef10 ^ (ulonglong)auStack_228;
  local_1d0 = param_3;
  local_1f0 = (undefined1 *)param_2;
  local_1e8 = param_1;
  local_1f8 = 0;
  local_a8 = 0xf;
  local_b0 = 0;
  local_c0 = 0;
  local_200 = 0xffffffffffffffff;
  local_1e0[0] = -1;
  local_204 = 0;
  local_208[0] = param_4;
  switch(*(undefined4 *)(param_2 + 4)) {
  case 0x10:
    if ((*(char *)(param_2 + 0x109) == '\0') || (*(longlong *)(param_2 + 0x58) == 0)) {
      iVar3 = *(int *)(param_2 + 0x90) - *(int *)(param_2 + 0x88);
      local_200 = CONCAT44(0xffffffff,iVar3);
      puVar6 = (uint *)&local_200;
      if (iVar3 <= (int)*param_3) {
        puVar6 = param_3;
      }
      *param_3 = *puVar6;
    }
    else {
      _Str = (char *)(param_2 + 0x48);
      if (0xf < *(ulonglong *)(param_2 + 0x60)) {
        _Str = *(char **)_Str;
      }
      iVar3 = atoi(_Str);
      dVar15 = (double)log2((double)iVar3);
      dVar15 = ceil(dVar15 + DAT_180e836b8);
      local_200 = CONCAT44(local_200._4_4_,(int)dVar15);
      puVar6 = (uint *)&local_200;
      if ((int)dVar15 <= (int)*param_3) {
        puVar6 = param_3;
      }
      *param_3 = *puVar6;
    }
    break;
  case 0x11:
    local_200 = 0xffffffff00000040;
    puVar6 = (uint *)&local_200;
    if (0x3f < (int)*param_3) {
      puVar6 = param_3;
    }
    *param_3 = *puVar6;
    break;
  default:
    *param_3 = 0xffffffff;
    break;
  case 0x13:
    local_1f0 = &local_e0;
    uVar9 = FUN_1800d77b0(&local_e0,param_2 + 0x48);
    uVar11 = FUN_1805fed20(local_1e8,uVar9);
    puVar14 = local_1e8;
    local_204 = uVar11;
    if (uVar11 != 0xffffffff) {
      if ((*(longlong *)(param_2 + 0x10) - *(longlong *)(param_2 + 8) & 0xfffffffffffffff8U) == 0) {
LAB_1805ff85d:
        if (uVar11 != 0xffffffff) {
LAB_1805ff862:
          puVar6 = &local_204;
          if ((int)uVar11 <= (int)*param_3) {
            puVar6 = param_3;
          }
          *param_3 = *puVar6;
          return;
        }
      }
      else if ((*(longlong *)(param_2 + 0x10) - *(longlong *)(param_2 + 8) >> 3 == 0) ||
              (lVar10 = **(longlong **)(param_2 + 8), *(int *)(lVar10 + 4) != 0xe)) {
        if ((1 < (ulonglong)(*(longlong *)(param_2 + 0x10) - *(longlong *)(param_2 + 8) >> 3)) &&
           (lVar10 = *(longlong *)(*(longlong *)(param_2 + 8) + 8), *(int *)(lVar10 + 4) == 0xe)) {
          if ((*(longlong *)(lVar10 + 0x10) - *(longlong *)(lVar10 + 8) & 0xfffffffffffffff8U) == 8)
          goto LAB_1805ff2d7;
          if (((1 < (ulonglong)(*(longlong *)(lVar10 + 0x10) - *(longlong *)(lVar10 + 8) >> 3)) &&
              ((lVar4 = **(longlong **)(lVar10 + 8), *(char *)(lVar4 + 0x109) != '\0' ||
               ((*(int *)(lVar4 + 4) == 0x13 &&
                (lVar4 = FUN_180676aa0(local_1e8 + 0x1a8,lVar4 + 0x48), lVar4 != 0)))))) &&
             ((lVar4 = *(longlong *)(*(longlong *)(lVar10 + 8) + 8),
              *(char *)(lVar4 + 0x109) != '\0' ||
              ((*(int *)(lVar4 + 4) == 0x13 &&
               (lVar4 = FUN_180676aa0(puVar14 + 0x1a8,lVar4 + 0x48), lVar4 != 0)))))) {
            lVar4 = **(longlong **)(lVar10 + 8);
            if (*(char *)(lVar4 + 0x109) == '\0') {
              puVar5 = (undefined4 *)FUN_1800d95e0(puVar14 + 0x1a8,lVar4 + 0x48);
              local_200 = CONCAT44(local_200._4_4_,*puVar5);
            }
            else {
              local_200 = CONCAT44(local_200._4_4_,*(undefined4 *)(lVar4 + 0xbc));
            }
            lVar4 = *(longlong *)(*(longlong *)(lVar10 + 8) + 8);
            if (*(char *)(lVar4 + 0x109) == '\0') {
              puVar6 = (uint *)FUN_1800d95e0(puVar14 + 0x1a8,lVar4 + 0x48);
              uVar11 = *puVar6;
            }
            else {
              uVar11 = *(uint *)(lVar4 + 0xbc);
            }
            local_e8 = 0xf;
            local_f0 = 0;
            local_100 = 0;
            local_204 = uVar11;
            FUN_180040550(&local_100,lVar10 + 0x48,0,0xffffffffffffffff);
            puVar14 = (undefined1 *)CONCAT71(uStack_ff,local_100);
            local_1e8 = &local_100;
            if (0xf < local_e8) {
              local_1e8 = puVar14;
            }
            puVar7 = &local_100;
            if (0xf < local_e8) {
              puVar7 = puVar14;
            }
            puVar12 = &local_100;
            if (0xf < local_e8) {
              puVar12 = puVar14;
            }
            lVar10 = (longlong)(puVar7 + local_f0) - (longlong)puVar12;
            if (puVar7 + local_f0 < puVar12) {
              lVar10 = 0;
            }
            if (lVar10 != 0) {
              lVar4 = 0;
              do {
                iVar3 = tolower((int)(char)puVar12[lVar4]);
                local_1e8[lVar4] = (char)iVar3;
                lVar4 = lVar4 + 1;
              } while (lVar4 != lVar10);
              puVar14 = (undefined1 *)CONCAT71(uStack_ff,local_100);
              param_3 = local_1d0;
              uVar11 = local_204;
            }
            puVar7 = &local_100;
            if (0xf < local_e8) {
              puVar7 = puVar14;
            }
            sVar13 = 2;
            if (local_f0 < 2) {
              sVar13 = local_f0;
            }
            if (sVar13 == 0) {
              uVar2 = 0;
            }
            else {
              uVar2 = memcmp(puVar7,&DAT_180ec5cec,sVar13);
              puVar14 = (undefined1 *)CONCAT71(uStack_ff,local_100);
            }
            if (uVar2 == 0) {
              if (local_f0 < 2) {
                uVar2 = 0xffffffff;
              }
              else {
                uVar2 = (uint)(local_f0 != 2);
              }
            }
            if ((uVar2 == 0) && ((uint)local_200 <= uVar11)) {
              uVar11 = (uVar11 - (uint)local_200) + 1;
            }
            else {
              puVar7 = &local_100;
              if (0xf < local_e8) {
                puVar7 = puVar14;
              }
              sVar13 = 6;
              if (local_f0 < 6) {
                sVar13 = local_f0;
              }
              if (sVar13 == 0) {
                uVar2 = 0;
              }
              else {
                uVar2 = memcmp(puVar7,"downto",sVar13);
              }
              if (uVar2 == 0) {
                if (local_f0 < 6) {
                  uVar2 = 0xffffffff;
                }
                else {
                  uVar2 = (uint)(local_f0 != 6);
                }
              }
              if ((uVar2 == 0) && (uVar11 <= (uint)local_200)) {
                uVar11 = ((uint)local_200 - uVar11) + 1;
              }
              else {
                uVar11 = 0;
              }
            }
            local_204 = uVar11;
            goto LAB_1805ff832;
          }
        }
      }
      else {
        if ((*(longlong *)(lVar10 + 0x10) - *(longlong *)(lVar10 + 8) & 0xfffffffffffffff8U) == 8) {
LAB_1805ff2d7:
          uVar11 = 1;
          local_204 = 1;
          goto LAB_1805ff862;
        }
        if (((1 < (ulonglong)(*(longlong *)(lVar10 + 0x10) - *(longlong *)(lVar10 + 8) >> 3)) &&
            ((lVar4 = **(longlong **)(lVar10 + 8), *(char *)(lVar4 + 0x109) != '\0' ||
             ((*(int *)(lVar4 + 4) == 0x13 &&
              (lVar4 = FUN_180676aa0(local_1e8 + 0x1a8,lVar4 + 0x48), lVar4 != 0)))))) &&
           ((lVar4 = *(longlong *)(*(longlong *)(lVar10 + 8) + 8), *(char *)(lVar4 + 0x109) != '\0'
            || ((*(int *)(lVar4 + 4) == 0x13 &&
                (lVar4 = FUN_180676aa0(puVar14 + 0x1a8,lVar4 + 0x48), lVar4 != 0)))))) {
          lVar4 = **(longlong **)(lVar10 + 8);
          if (*(char *)(lVar4 + 0x109) == '\0') {
            puVar5 = (undefined4 *)FUN_1800d95e0(puVar14 + 0x1a8,lVar4 + 0x48);
            local_200 = CONCAT44(local_200._4_4_,*puVar5);
          }
          else {
            local_200 = CONCAT44(local_200._4_4_,*(undefined4 *)(lVar4 + 0xbc));
          }
          lVar4 = *(longlong *)(*(longlong *)(lVar10 + 8) + 8);
          if (*(char *)(lVar4 + 0x109) == '\0') {
            puVar6 = (uint *)FUN_1800d95e0(puVar14 + 0x1a8,lVar4 + 0x48);
            uVar11 = *puVar6;
          }
          else {
            uVar11 = *(uint *)(lVar4 + 0xbc);
          }
          local_e8 = 0xf;
          local_f0 = 0;
          local_100 = 0;
          local_204 = uVar11;
          FUN_180040550(&local_100,lVar10 + 0x48,0,0xffffffffffffffff);
          puVar14 = (undefined1 *)CONCAT71(uStack_ff,local_100);
          local_1f0 = &local_100;
          if (0xf < local_e8) {
            local_1f0 = puVar14;
          }
          puVar7 = &local_100;
          if (0xf < local_e8) {
            puVar7 = puVar14;
          }
          puVar12 = &local_100;
          if (0xf < local_e8) {
            puVar12 = puVar14;
          }
          lVar10 = (longlong)(puVar7 + local_f0) - (longlong)puVar12;
          if (puVar7 + local_f0 < puVar12) {
            lVar10 = 0;
          }
          if (lVar10 != 0) {
            lVar4 = 0;
            do {
              iVar3 = tolower((int)(char)puVar12[lVar4]);
              local_1f0[lVar4] = (char)iVar3;
              lVar4 = lVar4 + 1;
            } while (lVar4 != lVar10);
            puVar14 = (undefined1 *)CONCAT71(uStack_ff,local_100);
            param_3 = local_1d0;
            uVar11 = local_204;
          }
          puVar7 = &local_100;
          if (0xf < local_e8) {
            puVar7 = puVar14;
          }
          sVar13 = 2;
          if (local_f0 < 2) {
            sVar13 = local_f0;
          }
          if (sVar13 == 0) {
            uVar2 = 0;
          }
          else {
            uVar2 = memcmp(puVar7,&DAT_180ec5cec,sVar13);
            puVar14 = (undefined1 *)CONCAT71(uStack_ff,local_100);
          }
          if (uVar2 == 0) {
            if (local_f0 < 2) {
              uVar2 = 0xffffffff;
            }
            else {
              uVar2 = (uint)(local_f0 != 2);
            }
          }
          if ((uVar2 == 0) && ((uint)local_200 <= uVar11)) {
            uVar11 = (uVar11 - (uint)local_200) + 1;
          }
          else {
            puVar7 = &local_100;
            if (0xf < local_e8) {
              puVar7 = puVar14;
            }
            sVar13 = 6;
            if (local_f0 < 6) {
              sVar13 = local_f0;
            }
            if (sVar13 == 0) {
              uVar2 = 0;
            }
            else {
              uVar2 = memcmp(puVar7,"downto",sVar13);
            }
            if (uVar2 == 0) {
              if (local_f0 < 6) {
                uVar2 = 0xffffffff;
              }
              else {
                uVar2 = (uint)(local_f0 != 6);
              }
            }
            if ((uVar2 == 0) && (uVar11 <= (uint)local_200)) {
              uVar11 = ((uint)local_200 - uVar11) + 1;
            }
            else {
              uVar11 = 0;
            }
          }
          local_204 = uVar11;
LAB_1805ff832:
          if (0xf < local_e8) {
            local_204 = uVar11;
            FUN_1800409e0(&local_100,CONCAT71(uStack_ff,local_100),local_e8 + 1);
          }
          local_100 = 0;
          local_f0 = 0;
          local_e8 = 0xf;
          goto LAB_1805ff85d;
        }
      }
    }
LAB_1805ff8d2:
    *param_3 = 0xffffffff;
    break;
  case 0x1c:
    local_204 = 0xffffffff;
    local_1f0 = local_1c0;
    FUN_1800d77b0(local_1c0,param_2 + 0x48);
    puVar8 = (undefined8 *)FUN_180600c30(local_a0);
    uVar11 = 1;
    local_1f8 = 1;
    local_1f0 = (undefined1 *)puVar8[2];
    if (0xf < (ulonglong)puVar8[3]) {
      puVar8 = (undefined8 *)*puVar8;
    }
    puVar14 = (undefined1 *)0xb;
    if (local_1f0 < (undefined1 *)0xb) {
      puVar14 = local_1f0;
    }
    if (puVar14 == (undefined1 *)0x0) {
      uVar2 = 0;
    }
    else {
      uVar2 = memcmp(puVar8,"to_unsigned",(size_t)puVar14);
    }
    if (uVar2 == 0) {
      if (local_1f0 < (undefined1 *)0xb) {
        uVar2 = 0xffffffff;
      }
      else {
        uVar2 = (uint)(local_1f0 != (undefined1 *)0xb);
      }
    }
    if (uVar2 == 0) {
LAB_1805ffc38:
      local_208[0] = '\x01';
      if ((*(longlong *)(param_2 + 0x10) - *(longlong *)(param_2 + 8) & 0xfffffffffffffff8U) != 0x10
         ) goto LAB_1805ffc4f;
    }
    else {
      local_1f0 = local_1a0;
      uVar9 = FUN_1800d77b0(local_1a0,param_2 + 0x48);
      uVar9 = FUN_180600c30(local_40,uVar9);
      uVar11 = 3;
      local_1f8 = 3;
      iVar3 = FUN_1800dc7d0(uVar9,"to_integer");
      if (iVar3 == 0) goto LAB_1805ffc38;
      local_1f0 = local_180;
      uVar9 = FUN_1800d77b0(local_180,param_2 + 0x48);
      uVar9 = FUN_180600c30(&local_e0,uVar9);
      uVar11 = 7;
      local_1f8 = 7;
      iVar3 = FUN_1800dc7d0(uVar9,"to_signed");
      if (iVar3 == 0) goto LAB_1805ffc38;
      local_1f0 = local_160;
      uVar9 = FUN_1800d77b0(local_160,param_2 + 0x48);
      uVar9 = FUN_180600c30(&local_60,uVar9);
      uVar11 = 0xf;
      local_1f8 = 0xf;
      iVar3 = FUN_1800dc7d0(uVar9,"conv_std_logic_vector");
      if (iVar3 == 0) goto LAB_1805ffc38;
      local_1f0 = local_140;
      uVar9 = FUN_1800d77b0(local_140,param_2 + 0x48);
      uVar9 = FUN_180600c30(&local_80,uVar9);
      uVar11 = 0x1f;
      local_1f8 = 0x1f;
      iVar3 = FUN_1800dc7d0(uVar9,"to_stdlogicvector");
      if (iVar3 == 0) goto LAB_1805ffc38;
      local_1f0 = local_120;
      uVar9 = FUN_1800d77b0(local_120,param_2 + 0x48);
      uVar9 = FUN_180600c30(&local_100,uVar9);
      uVar11 = 0x3f;
      local_1f8 = 0x3f;
      iVar3 = FUN_1800dc7d0(uVar9,"to_bitvector");
      if (iVar3 == 0) goto LAB_1805ffc38;
LAB_1805ffc4f:
      local_208[0] = '\0';
    }
    if ((uVar11 & 0x20) != 0) {
      uVar11 = uVar11 & 0xffffffdf;
      local_1f8 = uVar11;
      if (0xf < local_e8) {
        FUN_1800409e0(&local_100,CONCAT71(uStack_ff,local_100),local_e8 + 1);
      }
      local_e8 = 0xf;
      local_f0 = 0;
      local_100 = 0;
    }
    if ((uVar11 & 0x10) != 0) {
      uVar11 = uVar11 & 0xffffffef;
      local_1f8 = uVar11;
      if (0xf < local_68) {
        FUN_1800409e0(&local_80,CONCAT71(uStack_7f,local_80),local_68 + 1);
      }
      local_68 = 0xf;
      local_70 = 0;
      local_80 = 0;
    }
    if ((uVar11 & 8) != 0) {
      uVar11 = uVar11 & 0xfffffff7;
      local_1f8 = uVar11;
      if (0xf < local_48) {
        FUN_1800409e0(&local_60,CONCAT71(uStack_5f,local_60),local_48 + 1);
      }
      local_48 = 0xf;
      local_50 = 0;
      local_60 = 0;
    }
    if ((uVar11 & 4) != 0) {
      uVar11 = uVar11 & 0xfffffffb;
      local_1f8 = uVar11;
      if (0xf < local_c8) {
        FUN_1800409e0(&local_e0,CONCAT71(uStack_df,local_e0),local_c8 + 1);
      }
      local_c8 = 0xf;
      local_d0 = 0;
      local_e0 = 0;
    }
    if ((uVar11 & 2) != 0) {
      uVar11 = uVar11 & 0xfffffffd;
      local_1f8 = uVar11;
      FUN_18003efe0(local_40);
    }
    if ((uVar11 & 1) != 0) {
      local_1f8 = uVar11 & 0xfffffffe;
      FUN_18003efe0(local_a0);
    }
    if (local_208[0] != '\0') {
      lVar10 = *(longlong *)(*(longlong *)(param_2 + 8) + 8);
      if (*(int *)(lVar10 + 4) == 0x10) {
        local_204 = FUN_1808dafe0(lVar10,*(undefined1 *)(lVar10 + 0xa4));
LAB_1805ffecb:
        local_200 = (ulonglong)local_204;
      }
      else if (*(int *)(lVar10 + 4) == 0x13) {
        if ((*(longlong *)(lVar10 + 0x10) - *(longlong *)(lVar10 + 8) & 0xfffffffffffffff8U) == 0) {
          local_208[0] = '\0';
          local_1f0 = local_a0;
          uVar9 = FUN_1800d77b0(local_a0,lVar10 + 0x48);
          local_204 = FUN_1805fff90(local_1e8,uVar9,local_208);
          if (local_208[0] != '\0') goto LAB_1805ffecb;
        }
        else if (((*(longlong *)(lVar10 + 0x10) - *(longlong *)(lVar10 + 8) & 0xfffffffffffffff8U)
                  == 8) && (*(int *)(**(longlong **)(lVar10 + 8) + 4) == 0xad)) {
          local_1f0 = local_a0;
          uVar9 = FUN_1800d77b0(local_a0,**(longlong **)(lVar10 + 8) + 0x48);
          uVar9 = FUN_180600c30(local_40,uVar9);
          iVar3 = FUN_1800dc7d0(uVar9,"length");
          FUN_18003efe0(local_40);
          if (iVar3 == 0) {
            local_1f0 = local_a0;
            uVar9 = FUN_1800d77b0(local_a0,*(longlong *)(*(longlong *)(param_2 + 8) + 8) + 0x48);
            local_204 = FUN_1805fed20(local_1e8,uVar9);
            goto LAB_1805ffecb;
          }
        }
      }
    }
    puVar6 = &local_204;
    if ((int)local_200 <= (int)*param_3) {
      puVar6 = param_3;
    }
    *param_3 = *puVar6;
    break;
  case 0x22:
    puVar1 = *(undefined8 **)(param_2 + 0x10);
    local_204 = 0;
    for (puVar8 = *(undefined8 **)(param_2 + 8); puVar8 != puVar1; puVar8 = puVar8 + 1) {
      local_1e0[0] = -1;
      FUN_1805ff100(local_1e8,*puVar8,local_1e0,0);
      if (local_1e0[0] == -1) goto LAB_1805ff8d2;
      local_204 = local_204 + local_1e0[0];
    }
    puVar6 = &local_204;
    if ((int)local_204 <= (int)*param_3) {
      puVar6 = param_3;
    }
    *param_3 = *puVar6;
    break;
  case 0x24:
  case 0x2e:
  case 0x2f:
  case 0x30:
  case 0x31:
  case 0x32:
  case 0x33:
  case 0x42:
  case 0x43:
  case 0x44:
    FUN_1805ff100(param_1,**(undefined8 **)(param_2 + 8),param_3,0);
    break;
  case 0x25:
  case 0x26:
  case 0x27:
  case 0x28:
    puVar8 = *(undefined8 **)(param_2 + 8);
    puVar1 = *(undefined8 **)(param_2 + 0x10);
    while ((puVar8 != puVar1 && (FUN_1805ff100(local_1e8,*puVar8,param_3,0), *param_3 != 0xffffffff)
           )) {
      puVar8 = puVar8 + 1;
    }
    break;
  case 0x29:
  case 0x2a:
  case 0x2b:
  case 0x2c:
  case 0x2d:
  case 0x34:
  case 0x35:
  case 0x36:
  case 0x37:
  case 0x38:
  case 0x39:
  case 0x3a:
  case 0x3b:
  case 0x45:
  case 0x46:
  case 0x47:
    local_200 = 0xffffffff00000001;
    puVar6 = (uint *)&local_200;
    if (0 < (int)*param_3) {
      puVar6 = param_3;
    }
    *param_3 = *puVar6;
    break;
  case 0x3c:
  case 0x3d:
  case 0x3f:
  case 0x40:
  case 0x41:
    puVar1 = *(undefined8 **)(param_2 + 0x10);
    for (puVar8 = *(undefined8 **)(param_2 + 8); puVar8 != puVar1; puVar8 = puVar8 + 1) {
      FUN_1805ff100(local_1e8,*puVar8,param_3,0);
      if (*param_3 == 0xffffffff) {
        return;
      }
    }
    if ((local_208[0] == '\0') && (*(int *)((longlong)local_1f0 + 4) - 0x3cU < 2)) {
      *param_3 = *param_3 + 1;
    }
    break;
  case 0x3e:
    local_200._0_4_ = 0;
    local_200._4_4_ = 0xffffffff;
    puVar1 = *(undefined8 **)(param_2 + 0x10);
    local_200._0_4_ = 0;
    for (puVar8 = *(undefined8 **)(param_2 + 8); puVar8 != puVar1; puVar8 = puVar8 + 1) {
      local_204 = 0xffffffff;
      FUN_1805ff100(local_1e8,*puVar8,&local_204,0);
      if (local_204 == 0xffffffff) {
        return;
      }
      local_200._0_4_ = (uint)local_200 + local_204;
    }
    puVar6 = (uint *)&local_200;
    if ((int)(uint)local_200 <= (int)*param_3) {
      puVar6 = param_3;
    }
    *param_3 = *puVar6;
  }
  return;
}



===== 0x180600050 180600050 FUN_180600050 =====

/* WARNING: Function: __security_check_cookie replaced with injection: security_check_cookie */
/* WARNING: Removing unreachable block (ram,0x000180600591) */

void FUN_180600050(longlong param_1,longlong param_2)

{
  void *pvVar1;
  ulonglong uVar2;
  char cVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  int *piVar7;
  undefined4 *puVar8;
  size_t sVar9;
  void *_Memory;
  byte *pbVar10;
  byte *pbVar11;
  longlong *plVar12;
  longlong lVar13;
  byte *pbVar14;
  longlong lVar15;
  byte *pbVar16;
  undefined1 auStack_e8 [32];
  int local_c8 [2];
  longlong local_c0;
  longlong local_b8;
  byte *local_b0;
  byte *local_a8;
  byte *local_a0;
  byte *local_98;
  byte *local_90;
  byte *local_88;
  byte *local_80;
  byte *local_78;
  byte *local_70;
  undefined8 local_68;
  byte local_60;
  undefined7 uStack_5f;
  ulonglong local_50;
  ulonglong local_48;
  byte local_40;
  undefined7 uStack_3f;
  ulonglong local_30;
  ulonglong local_28;
  ulonglong local_20;
  
  if (param_2 != 0) {
    local_68 = 0xfffffffffffffffe;
    local_20 = DAT_18131ef10 ^ (ulonglong)auStack_e8;
    local_c0 = param_1;
    local_b8 = param_2;
    if (*(int *)(param_2 + 4) == 0x76) {
      local_90 = *(byte **)(param_2 + 8);
      pbVar16 = *(byte **)(param_2 + 0x10);
      local_78 = pbVar16;
      if (local_90 != pbVar16) {
        do {
          local_a0 = *(byte **)local_90;
          lVar13 = local_c0;
          if (*(int *)(local_a0 + 4) == 0xa4) {
            pbVar10 = *(byte **)(local_a0 + 8);
            pbVar11 = *(byte **)(local_a0 + 0x10);
            local_a8 = pbVar11;
            local_80 = pbVar10;
            if (pbVar10 != pbVar11) {
              do {
                lVar15 = 0;
                local_b0 = *(byte **)pbVar10;
                if (local_b0[0x109] == 0) {
                  local_80 = pbVar10;
                  if (*(longlong *)(local_b0 + 0x88) == *(longlong *)(local_b0 + 0x90)) {
                    local_c8[0] = FUN_1805fd0b0(lVar13,local_b0);
                  }
                  else {
                    local_c8[0] = (int)*(longlong *)(local_b0 + 0x90) - *(int *)(local_b0 + 0x88);
                  }
                  local_28 = 0xf;
                  local_30 = 0;
                  local_40 = 0;
                  FUN_180040550(&local_40,local_b0 + 0x158,0,0xffffffffffffffff);
                  pbVar16 = (byte *)CONCAT71(uStack_3f,local_40);
                  local_88 = &local_40;
                  if (0xf < local_28) {
                    local_88 = pbVar16;
                  }
                  pbVar14 = &local_40;
                  if (0xf < local_28) {
                    pbVar14 = pbVar16;
                  }
                  lVar13 = (longlong)(local_88 + local_30) - (longlong)pbVar14;
                  if (local_88 + local_30 < pbVar14) {
                    lVar13 = lVar15;
                  }
                  if (lVar13 != 0) {
                    do {
                      iVar4 = tolower((int)(char)pbVar14[lVar15]);
                      local_88[lVar15] = (byte)iVar4;
                      lVar15 = lVar15 + 1;
                    } while (lVar15 != lVar13);
                    pbVar16 = (byte *)CONCAT71(uStack_3f,local_40);
                    pbVar10 = local_80;
                    pbVar11 = local_a8;
                  }
                  pbVar14 = &local_40;
                  if (0xf < local_28) {
                    pbVar14 = pbVar16;
                  }
                  sVar9 = 6;
                  if (local_30 < 6) {
                    sVar9 = local_30;
                  }
                  if (sVar9 == 0) {
                    uVar6 = 0;
                  }
                  else {
                    uVar6 = memcmp(pbVar14,"signed",sVar9);
                  }
                  if (uVar6 == 0) {
                    if (local_30 < 6) {
                      uVar6 = 0xffffffff;
                    }
                    else {
                      uVar6 = (uint)(local_30 != 6);
                    }
                  }
                  piVar7 = (int *)FUN_1805fcfc0(local_c0 + 0x188,local_b0 + 0x48);
                  *piVar7 = local_c8[0];
                  *(bool *)(piVar7 + 1) = uVar6 == 0;
                  if (0xf < local_28) {
                    FUN_1800409e0(&local_40,CONCAT71(uStack_3f,local_40),local_28 + 1);
                  }
                  local_28 = 0xf;
                  local_30 = 0;
                  local_40 = 0;
                  lVar13 = local_c0;
                }
                pbVar10 = pbVar10 + 8;
                pbVar16 = local_78;
                local_80 = pbVar10;
              } while (pbVar10 != pbVar11);
            }
          }
          if (*(int *)(local_a0 + 4) == 0xa3) {
            pbVar10 = *(byte **)(local_a0 + 8);
            pbVar11 = *(byte **)(local_a0 + 0x10);
            local_b0 = pbVar10;
            local_70 = pbVar11;
            if (pbVar10 != pbVar11) {
              do {
                lVar15 = local_c0;
                pbVar16 = *(byte **)pbVar10;
                local_b0 = pbVar10;
                local_80 = pbVar16;
                if (pbVar16[0x109] == 0) {
                  if (*(longlong *)(pbVar16 + 0x88) == *(longlong *)(pbVar16 + 0x90)) {
                    local_c8[0] = FUN_1805fd0b0(lVar13,pbVar16);
                  }
                  else {
                    local_c8[0] = (int)*(longlong *)(pbVar16 + 0x90) - *(int *)(pbVar16 + 0x88);
                  }
                  lVar13 = 0;
                  local_48 = 0xf;
                  local_50 = 0;
                  local_60 = 0;
                  FUN_180040550(&local_60,local_80 + 0x158,0,0xffffffffffffffff);
                  pbVar16 = (byte *)CONCAT71(uStack_5f,local_60);
                  local_98 = &local_60;
                  if (0xf < local_48) {
                    local_98 = pbVar16;
                  }
                  pbVar14 = &local_60;
                  if (0xf < local_48) {
                    pbVar14 = pbVar16;
                  }
                  lVar15 = (longlong)(local_98 + local_50) - (longlong)pbVar14;
                  if (local_98 + local_50 < pbVar14) {
                    lVar15 = lVar13;
                  }
                  if (lVar15 != 0) {
                    do {
                      iVar4 = tolower((int)(char)pbVar14[lVar13]);
                      local_98[lVar13] = (byte)iVar4;
                      lVar13 = lVar13 + 1;
                    } while (lVar13 != lVar15);
                    pbVar16 = (byte *)CONCAT71(uStack_5f,local_60);
                    pbVar11 = local_70;
                    pbVar10 = local_b0;
                  }
                  pbVar14 = &local_60;
                  if (0xf < local_48) {
                    pbVar14 = pbVar16;
                  }
                  sVar9 = 6;
                  if (local_50 < 6) {
                    sVar9 = local_50;
                  }
                  if (sVar9 == 0) {
                    uVar6 = 0;
                  }
                  else {
                    uVar6 = memcmp(pbVar14,"signed",sVar9);
                  }
                  if (uVar6 == 0) {
                    if (local_50 < 6) {
                      uVar6 = 0xffffffff;
                    }
                    else {
                      uVar6 = (uint)(local_50 != 6);
                    }
                  }
                  piVar7 = (int *)FUN_1805fcfc0(local_c0 + 0x198,local_80 + 0x48);
                  *piVar7 = local_c8[0];
                  *(bool *)(piVar7 + 1) = uVar6 == 0;
                  if (0xf < local_48) {
                    pvVar1 = (void *)CONCAT71(uStack_5f,local_60);
                    _Memory = pvVar1;
                    if (0xfff < local_48 + 1) {
                      if ((local_60 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      _Memory = *(void **)((longlong)pvVar1 - 8);
                      if (pvVar1 <= _Memory) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if ((ulonglong)((longlong)pvVar1 - (longlong)_Memory) < 8) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                      if (0x27 < (ulonglong)((longlong)pvVar1 - (longlong)_Memory)) {
                    /* WARNING: Subroutine does not return */
                        _invalid_parameter_noinfo_noreturn();
                      }
                    }
                    free(_Memory);
                  }
                  local_48 = 0xf;
                  local_50 = 0;
                  local_60 = 0;
                  local_b0 = pbVar10;
                }
                else {
                  plVar12 = *(longlong **)(pbVar16 + 8);
                  local_88 = (byte *)0x0;
                  pbVar14 = (byte *)((ulonglong)
                                     ((longlong)*(longlong **)(pbVar16 + 0x10) +
                                     (7 - (longlong)plVar12)) >> 3);
                  if (*(longlong **)(pbVar16 + 0x10) < plVar12) {
                    pbVar14 = (byte *)0x0;
                  }
                  local_98 = pbVar14;
                  if (pbVar14 != (byte *)0x0) {
                    do {
                      if (*(int *)(*plVar12 + 4) != 0xe) {
                        local_c8[0] = -1;
                        cVar3 = FUN_1804b78d0(lVar15,*plVar12,local_c8);
                        pbVar14 = local_98;
                        if (cVar3 != '\0') {
                          piVar7 = (int *)FUN_1800d95e0(lVar15 + 0x1a8,pbVar16 + 0x48);
                          *piVar7 = local_c8[0];
                          pbVar14 = local_98;
                        }
                      }
                      plVar12 = plVar12 + 1;
                      local_88 = local_88 + 1;
                      pbVar11 = local_70;
                    } while (local_88 != pbVar14);
                  }
                }
                pbVar10 = local_b0 + 8;
                pbVar16 = local_78;
                lVar13 = local_c0;
                local_b0 = pbVar10;
              } while (pbVar10 != pbVar11);
            }
          }
          local_90 = local_90 + 8;
        } while (local_90 != pbVar16);
      }
    }
    lVar15 = local_b8;
    pbVar16 = (byte *)0x0;
    uVar6 = 0;
    lVar13 = local_c0;
    if ((*(int *)(local_b8 + 4) == 0x78) && (*(char *)(local_b8 + 0x109) == '\0')) {
      if (*(longlong *)(local_b8 + 0x88) == *(longlong *)(local_b8 + 0x90)) {
        local_c8[0] = FUN_1805fd0b0(local_c0,local_b8);
      }
      else {
        local_c8[0] = (int)*(longlong *)(local_b8 + 0x90) - *(int *)(local_b8 + 0x88);
      }
      local_48 = 0xf;
      local_50 = 0;
      local_60 = 0;
      FUN_180040550(&local_60,lVar15 + 0x158,0,0xffffffffffffffff);
      pbVar10 = (byte *)CONCAT71(uStack_5f,local_60);
      local_a0 = &local_60;
      if (0xf < local_48) {
        local_a0 = pbVar10;
      }
      local_a8 = &local_60;
      if (0xf < local_48) {
        local_a8 = pbVar10;
      }
      pbVar11 = local_a0 + local_50 + -(longlong)local_a8;
      if (local_a0 + local_50 < local_a8) {
        pbVar11 = pbVar16;
      }
      pbVar14 = pbVar16;
      if (pbVar11 != (byte *)0x0) {
        do {
          iVar4 = tolower((int)(char)pbVar14[(longlong)local_a8]);
          pbVar14[(longlong)local_a0] = (byte)iVar4;
          pbVar14 = pbVar14 + 1;
        } while (pbVar14 != pbVar11);
        pbVar10 = (byte *)CONCAT71(uStack_5f,local_60);
      }
      uVar2 = local_50;
      pbVar11 = &local_60;
      if (0xf < local_48) {
        pbVar11 = pbVar10;
      }
      sVar9 = 6;
      if (local_50 < 6) {
        sVar9 = local_50;
      }
      uVar5 = uVar6;
      if (sVar9 != 0) {
        uVar5 = memcmp(pbVar11,"signed",sVar9);
      }
      lVar13 = local_c0;
      if (uVar5 == 0) {
        if (uVar2 < 6) {
          uVar5 = 0xffffffff;
        }
        else {
          uVar5 = (uint)(uVar2 != 6);
        }
      }
      piVar7 = (int *)FUN_1805fcfc0(local_c0 + 0x158,local_b8 + 0x48);
      *piVar7 = local_c8[0];
      *(bool *)(piVar7 + 1) = uVar5 == 0;
      if (0xf < local_48) {
        FUN_1800409e0(&local_60,CONCAT71(uStack_5f,local_60),local_48 + 1);
      }
      local_48 = 0xf;
      local_50 = 0;
      local_60 = 0;
    }
    lVar15 = local_b8;
    if (*(int *)(local_b8 + 4) == 10) {
      if (*(char *)(local_b8 + 0x109) == '\0') {
        if (*(char *)(local_b8 + 0x10e) == '\0') {
          if (*(longlong *)(local_b8 + 0x88) == *(longlong *)(local_b8 + 0x90)) {
            local_c8[0] = FUN_1805fd0b0(lVar13,local_b8);
          }
          else {
            local_c8[0] = (int)*(longlong *)(local_b8 + 0x90) - *(int *)(local_b8 + 0x88);
          }
          local_48 = 0xf;
          local_50 = 0;
          local_60 = 0;
          FUN_180040550(&local_60,lVar15 + 0x158,0,0xffffffffffffffff);
          pbVar10 = (byte *)CONCAT71(uStack_5f,local_60);
          local_a8 = &local_60;
          if (0xf < local_48) {
            local_a8 = pbVar10;
          }
          local_78 = &local_60;
          if (0xf < local_48) {
            local_78 = pbVar10;
          }
          pbVar11 = local_a8 + local_50 + -(longlong)local_78;
          if (local_a8 + local_50 < local_78) {
            pbVar11 = pbVar16;
          }
          pbVar14 = pbVar16;
          if (pbVar11 != (byte *)0x0) {
            do {
              iVar4 = tolower((int)(char)pbVar14[(longlong)local_78]);
              pbVar14[(longlong)local_a8] = (byte)iVar4;
              pbVar14 = pbVar14 + 1;
            } while (pbVar14 != pbVar11);
            pbVar10 = (byte *)CONCAT71(uStack_5f,local_60);
            lVar13 = local_c0;
          }
          uVar2 = local_50;
          pbVar11 = &local_60;
          if (0xf < local_48) {
            pbVar11 = pbVar10;
          }
          sVar9 = 6;
          if (local_50 < 6) {
            sVar9 = local_50;
          }
          uVar5 = uVar6;
          if (sVar9 != 0) {
            uVar5 = memcmp(pbVar11,"signed",sVar9);
          }
          if (uVar5 == 0) {
            if (uVar2 < 6) {
              uVar5 = 0xffffffff;
            }
            else {
              uVar5 = (uint)(uVar2 != 6);
            }
          }
          piVar7 = (int *)FUN_1805fcfc0(lVar13 + 0x168,local_b8 + 0x48);
          *piVar7 = local_c8[0];
          *(bool *)(piVar7 + 1) = uVar5 == 0;
          if (0xf < local_48) {
            FUN_1800409e0(&local_60,CONCAT71(uStack_5f,local_60),local_48 + 1);
          }
          local_48 = 0xf;
          local_50 = 0;
          local_60 = 0;
          lVar15 = local_b8;
        }
        else {
          puVar8 = (undefined4 *)FUN_1805fcfc0(lVar13 + 0x168,local_b8 + 0x48);
          *puVar8 = 1;
          *(undefined1 *)(puVar8 + 1) = 0;
        }
      }
      else {
        plVar12 = *(longlong **)(local_b8 + 8);
        local_b0 = (byte *)0x0;
        pbVar10 = (byte *)((ulonglong)
                           ((longlong)*(longlong **)(local_b8 + 0x10) + (7 - (longlong)plVar12)) >>
                          3);
        if (*(longlong **)(local_b8 + 0x10) < plVar12) {
          pbVar10 = pbVar16;
        }
        local_90 = pbVar10;
        if (pbVar10 != (byte *)0x0) {
          do {
            if (*(int *)(*plVar12 + 4) != 0xe) {
              local_c8[0] = -1;
              cVar3 = FUN_1804b78d0(lVar13,*plVar12,local_c8);
              pbVar10 = local_90;
              if (cVar3 != '\0') {
                piVar7 = (int *)FUN_1800d95e0(lVar13 + 0x1a8,local_b8 + 0x48);
                *piVar7 = local_c8[0];
                pbVar10 = local_90;
              }
            }
            plVar12 = plVar12 + 1;
            local_b0 = local_b0 + 1;
            lVar15 = local_b8;
          } while (local_b0 != pbVar10);
        }
      }
    }
    if ((*(int *)(lVar15 + 4) == 0x79) && (*(char *)(lVar15 + 0x109) == '\0')) {
      if (*(longlong *)(lVar15 + 0x88) == *(longlong *)(lVar15 + 0x90)) {
        local_c8[0] = FUN_1805fd0b0(lVar13,lVar15);
      }
      else {
        local_c8[0] = (int)*(longlong *)(lVar15 + 0x90) - *(int *)(lVar15 + 0x88);
      }
      local_48 = 0xf;
      local_50 = 0;
      local_60 = 0;
      FUN_180040550(&local_60,lVar15 + 0x158,0,0xffffffffffffffff);
      pbVar10 = (byte *)CONCAT71(uStack_5f,local_60);
      local_a8 = &local_60;
      if (0xf < local_48) {
        local_a8 = pbVar10;
      }
      local_78 = &local_60;
      if (0xf < local_48) {
        local_78 = pbVar10;
      }
      pbVar11 = local_a8 + local_50 + -(longlong)local_78;
      if (local_a8 + local_50 < local_78) {
        pbVar11 = pbVar16;
      }
      pbVar14 = pbVar16;
      if (pbVar11 != (byte *)0x0) {
        do {
          iVar4 = tolower((int)(char)local_78[(longlong)pbVar14]);
          local_a8[(longlong)pbVar14] = (byte)iVar4;
          pbVar14 = pbVar14 + 1;
        } while (pbVar14 != pbVar11);
        pbVar10 = (byte *)CONCAT71(uStack_5f,local_60);
        lVar13 = local_c0;
      }
      uVar2 = local_50;
      pbVar11 = &local_60;
      if (0xf < local_48) {
        pbVar11 = pbVar10;
      }
      sVar9 = 6;
      if (local_50 < 6) {
        sVar9 = local_50;
      }
      if (sVar9 != 0) {
        uVar6 = memcmp(pbVar11,"signed",sVar9);
      }
      if (uVar6 == 0) {
        if (uVar2 < 6) {
          uVar6 = 0xffffffff;
        }
        else {
          uVar6 = (uint)(uVar2 != 6);
        }
      }
      piVar7 = (int *)FUN_1805fcfc0(lVar13 + 0x178,local_b8 + 0x48);
      *piVar7 = local_c8[0];
      *(bool *)(piVar7 + 1) = uVar6 == 0;
      if (0xf < local_48) {
        FUN_1800409e0(&local_60,CONCAT71(uStack_5f,local_60),local_48 + 1);
      }
      local_48 = 0xf;
      local_50 = 0;
      local_60 = 0;
      lVar15 = local_b8;
    }
    if (*(longlong *)(lVar15 + 0x10) - *(longlong *)(lVar15 + 8) >> 3 != 0) {
      do {
        FUN_180600050(lVar13,*(undefined8 *)(*(longlong *)(lVar15 + 8) + (longlong)pbVar16 * 8));
        pbVar16 = pbVar16 + 1;
      } while (pbVar16 < (byte *)(*(longlong *)(lVar15 + 0x10) - *(longlong *)(lVar15 + 8) >> 3));
    }
  }
  return;
}



===== 0x1805fee80 1805fee80 FUN_1805fee80 =====

/* WARNING: Function: __security_check_cookie replaced with injection: security_check_cookie */

ulonglong FUN_1805fee80(longlong param_1,undefined8 *param_2)

{
  longlong lVar1;
  ulonglong uVar2;
  byte bVar3;
  undefined1 auStack_48 [32];
  undefined8 local_28;
  undefined8 *local_20;
  ulonglong local_18;
  
  local_28 = 0xfffffffffffffffe;
  local_18 = DAT_18131ef10 ^ (ulonglong)auStack_48;
  local_20 = param_2;
  lVar1 = FUN_180676aa0(param_1 + 0x168);
  if (lVar1 == 0) {
    lVar1 = FUN_180676aa0(param_1 + 0x188,param_2);
    if (lVar1 == 0) {
      lVar1 = FUN_180676aa0(param_1 + 0x198,param_2);
      if (lVar1 == 0) {
        lVar1 = FUN_180676aa0(param_1 + 0x158,param_2);
        if (lVar1 == 0) {
          lVar1 = FUN_180676aa0(param_1 + 0x178,param_2);
          if (lVar1 == 0) {
            uVar2 = FUN_18003efe0(param_2);
            return uVar2 & 0xffffffffffffff00;
          }
          param_1 = param_1 + 0x178;
        }
        else {
          param_1 = param_1 + 0x158;
        }
      }
      else {
        param_1 = param_1 + 0x198;
      }
      lVar1 = FUN_1805fcfc0(param_1,param_2);
      bVar3 = *(byte *)(lVar1 + 4);
      FUN_18003efe0(param_2);
      goto LAB_1805fef02;
    }
    param_1 = param_1 + 0x188;
  }
  else {
    param_1 = param_1 + 0x168;
  }
  lVar1 = FUN_1805fcfc0(param_1);
  bVar3 = *(byte *)(lVar1 + 4);
  if (0xf < (ulonglong)param_2[3]) {
    FUN_1800409e0(param_2,*param_2,param_2[3] + 1);
  }
  param_2[3] = 0xf;
  param_2[2] = 0;
  *(undefined1 *)param_2 = 0;
LAB_1805fef02:
  return (ulonglong)bVar3;
}



===== 0x1805fff90 1805fff90 FUN_1805fff90 =====

/* WARNING: Function: __security_check_cookie replaced with injection: security_check_cookie */

undefined4 FUN_1805fff90(longlong param_1,undefined8 *param_2,undefined1 *param_3)

{
  undefined4 uVar1;
  longlong lVar2;
  undefined4 *puVar3;
  undefined1 auStack_58 [32];
  undefined8 local_38;
  undefined8 *local_30;
  ulonglong local_28;
  
  local_38 = 0xfffffffffffffffe;
  local_28 = DAT_18131ef10 ^ (ulonglong)auStack_58;
  local_30 = param_2;
  lVar2 = FUN_180676aa0(param_1 + 0x1a8);
  if (lVar2 == 0) {
    *param_3 = 0;
    if (0xf < (ulonglong)param_2[3]) {
      FUN_1800409e0(param_2,*param_2,param_2[3] + 1);
    }
    uVar1 = 0xffffffff;
  }
  else {
    *param_3 = 1;
    puVar3 = (undefined4 *)FUN_1800d95e0(param_1 + 0x1a8,param_2);
    uVar1 = *puVar3;
    if (0xf < (ulonglong)param_2[3]) {
      FUN_1800409e0(param_2,*param_2,param_2[3] + 1);
    }
  }
  param_2[3] = 0xf;
  param_2[2] = 0;
  *(undefined1 *)param_2 = 0;
  return uVar1;
}


