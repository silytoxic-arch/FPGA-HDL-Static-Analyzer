Target: T-Linter.dll; SHA256: 60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0

===== 0x1806326d0 1806326d0 FUN_1806326d0 =====

/* WARNING: Function: __security_check_cookie replaced with injection: security_check_cookie */
/* WARNING: Removing unreachable block (ram,0x000180632ef7) */

void FUN_1806326d0(longlong param_1)

{
  char cVar1;
  undefined4 uVar2;
  undefined8 *puVar3;
  undefined8 *puVar4;
  longlong lVar5;
  longlong *plVar6;
  undefined8 *puVar7;
  void *pvVar8;
  longlong *plVar9;
  undefined8 *puVar10;
  ulonglong uVar11;
  undefined1 *puVar12;
  longlong *plVar13;
  void *_Memory;
  undefined1 *_Dst;
  undefined1 *puVar14;
  undefined1 auStack_2a8 [32];
  longlong local_288;
  undefined1 local_280 [16];
  undefined8 local_270;
  undefined8 local_268;
  undefined1 local_260 [16];
  undefined8 local_250;
  undefined8 local_248;
  undefined1 local_240 [16];
  undefined8 local_230;
  undefined8 local_228;
  undefined8 local_220;
  undefined1 *local_218;
  undefined1 *local_210;
  undefined1 *local_208;
  undefined1 local_200;
  undefined7 uStack_1ff;
  ulonglong local_1f0;
  ulonglong local_1e8;
  undefined1 local_1e0;
  undefined7 uStack_1df;
  undefined8 local_1d0;
  ulonglong local_1c8;
  undefined1 local_1c0;
  undefined7 uStack_1bf;
  undefined8 local_1b0;
  ulonglong local_1a8;
  undefined1 local_1a0;
  undefined7 uStack_19f;
  undefined8 local_190;
  ulonglong local_188;
  byte local_180;
  undefined7 uStack_17f;
  undefined8 local_170;
  ulonglong local_168;
  longlong *local_160;
  undefined8 uStack_158;
  longlong local_150;
  undefined1 local_148;
  undefined8 local_138;
  undefined8 local_130;
  undefined1 local_128;
  undefined7 uStack_127;
  undefined8 local_118;
  ulonglong local_110;
  undefined1 local_108;
  undefined7 uStack_107;
  undefined8 local_f8;
  ulonglong local_f0;
  undefined1 local_e8;
  undefined7 uStack_e7;
  ulonglong local_d8;
  ulonglong local_d0;
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
  
  local_220 = 0xfffffffffffffffe;
  local_38 = DAT_18131ef10 ^ (ulonglong)auStack_2a8;
  puVar3 = *(undefined8 **)(param_1 + 0x78);
  puVar4 = (undefined8 *)*puVar3;
  local_288 = param_1;
  do {
    if (puVar4 == puVar3) {
      return;
    }
    local_168 = 0xf;
    local_170 = 0;
    local_180 = 0;
    FUN_180040550(&local_180,puVar4 + 4,0,0xffffffffffffffff);
    FUN_1801acf00(&local_160,puVar4 + 8);
    lVar5 = *local_160;
    if (lVar5 == 0) {
      if (local_160 != (longlong *)0x0) {
        uVar11 = local_150 - (longlong)local_160 >> 3;
        if (0x1fffffffffffffff < uVar11) {
                    /* WARNING: Subroutine does not return */
          _invalid_parameter_noinfo_noreturn();
        }
        plVar13 = local_160;
        if (0xfff < uVar11 * 8) {
          if (((ulonglong)local_160 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          plVar13 = (longlong *)local_160[-1];
          if (local_160 <= plVar13) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if ((ulonglong)((longlong)local_160 - (longlong)plVar13) < 8) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if (0x27 < (ulonglong)((longlong)local_160 - (longlong)plVar13)) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
        }
        free(plVar13);
        local_160 = (longlong *)0x0;
        uStack_158 = 0;
        local_150 = 0;
      }
      if (0xf < local_168) {
        pvVar8 = (void *)CONCAT71(uStack_17f,local_180);
        _Memory = pvVar8;
        if (0xfff < local_168 + 1) {
          if ((local_180 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          _Memory = *(void **)((longlong)pvVar8 - 8);
          if (pvVar8 <= _Memory) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if ((ulonglong)((longlong)pvVar8 - (longlong)_Memory) < 8) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if (0x27 < (ulonglong)((longlong)pvVar8 - (longlong)_Memory)) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
        }
        free(_Memory);
      }
    }
    else {
      cVar1 = *(char *)((longlong)DAT_18135c448[1] + 0x19);
      plVar13 = (longlong *)DAT_18135c448[1];
      while (cVar1 == '\0') {
        FUN_1803e50c0(&DAT_18135c448,plVar13[2]);
        plVar6 = (longlong *)*plVar13;
        free(plVar13);
        plVar13 = plVar6;
        cVar1 = *(char *)((longlong)plVar6 + 0x19);
      }
      DAT_18135c448[1] = DAT_18135c448;
      *DAT_18135c448 = DAT_18135c448;
      DAT_18135c448[2] = DAT_18135c448;
      DAT_18135c450 = 0;
      DAT_18135c2e0 = 0;
      FUN_180632fc0(lVar5);
      puVar7 = DAT_18135c448;
      if (DAT_18135c450 != 0) {
        plVar13 = (longlong *)*DAT_18135c448;
        while (plVar13 != puVar7) {
          if ((char)plVar13[5] == '\0') {
            local_218 = local_280;
            local_210 = local_260;
            local_208 = local_240;
            local_268 = 0xf;
            local_270 = 0;
            local_280[0] = 0;
            FUN_180040680(local_280,&DAT_180de47b9,0);
            local_248 = 0xf;
            local_250 = 0;
            local_260[0] = 0;
            FUN_180040680(local_260,&DAT_180e3dc90,1);
            local_228 = 0xf;
            local_230 = 0;
            local_240[0] = 0;
            FUN_180040550(local_240,plVar13[4] + 0x48,0,0xffffffffffffffff);
            FUN_1804df040(&local_108,local_240,local_260,local_280);
            local_1e8 = 0xf;
            local_1f0 = 0;
            local_200 = 0;
            FUN_180040550(&local_200,param_1 + 0x10,0,0xffffffffffffffff);
            FUN_1800d37b0(&local_128,&DAT_180f066c0,&local_108);
            local_188 = 0xf;
            local_190 = 0;
            local_1a0 = 0;
            FUN_180040550(&local_1a0,lVar5 + 0x1e0,0,0xffffffffffffffff);
            uVar2 = *(undefined4 *)(plVar13[4] + 0x214);
            local_1a8 = 0xf;
            local_1b0 = 0;
            local_1c0 = 0;
            FUN_180040680(&local_1c0,"Warning",7);
            local_1c8 = 0xf;
            local_1d0 = 0;
            local_1e0 = 0;
            FUN_180040680(&local_1e0,&DAT_180de47b9,0);
            uVar11 = local_1f0;
            local_130 = 0xf;
            local_138 = 0;
            local_148 = 0;
            local_d0 = 0xf;
            local_d8 = 0;
            local_e8 = 0;
            if (local_1f0 == 0xffffffffffffffff) {
              std::_Xlength_error("string too long");
              return;
            }
            if (local_1f0 < 0x10) {
              if (local_1f0 == 0) {
                local_d8 = local_1f0;
                local_e8 = 0;
                goto LAB_180632b2b;
              }
LAB_180632b30:
              puVar12 = &local_200;
              if (0xf < local_1e8) {
                puVar12 = (undefined1 *)CONCAT71(uStack_1ff,local_200);
              }
              puVar14 = (undefined1 *)CONCAT71(uStack_e7,local_e8);
              _Dst = &local_e8;
              if (0xf < local_d0) {
                _Dst = puVar14;
              }
              if (uVar11 != 0) {
                memcpy(_Dst,puVar12,uVar11);
                puVar14 = (undefined1 *)CONCAT71(uStack_e7,local_e8);
              }
              local_d8 = uVar11;
              puVar12 = &local_e8;
              if (0xf < local_d0) {
                puVar12 = puVar14;
              }
              puVar12[uVar11] = 0;
            }
            else {
              FUN_18003f980(&local_e8,local_1f0,0);
LAB_180632b2b:
              if (uVar11 != 0) goto LAB_180632b30;
            }
            local_b0 = 0xf;
            local_b8 = 0;
            local_c8[0] = 0;
            FUN_180040550(local_c8,&local_128,0,0xffffffffffffffff);
            local_90 = 0xf;
            local_98 = 0;
            local_a8[0] = 0;
            FUN_180040550(local_a8,&local_1a0,0,0xffffffffffffffff);
            local_68 = 0xf;
            local_70 = 0;
            local_80[0] = 0;
            local_88 = uVar2;
            FUN_180040550(local_80,&local_1c0,0,0xffffffffffffffff);
            local_48 = 0xf;
            local_50 = 0;
            local_60[0] = 0;
            FUN_180040550(local_60,&local_1e0,0,0xffffffffffffffff);
            FUN_18049cca0(&DAT_18136fd88,&local_e8);
            FUN_18049bf90(&local_e8);
            local_130 = 0xf;
            local_138 = 0;
            local_148 = 0;
            if (0xf < local_1c8) {
              FUN_1800409e0(&local_1e0,CONCAT71(uStack_1df,local_1e0),local_1c8 + 1);
            }
            local_1c8 = 0xf;
            local_1d0 = 0;
            local_1e0 = 0;
            if (0xf < local_1a8) {
              FUN_1800409e0(&local_1c0,CONCAT71(uStack_1bf,local_1c0),local_1a8 + 1);
            }
            local_1a8 = 0xf;
            local_1b0 = 0;
            local_1c0 = 0;
            if (0xf < local_188) {
              FUN_1800409e0(&local_1a0,CONCAT71(uStack_19f,local_1a0),local_188 + 1);
            }
            local_188 = 0xf;
            local_190 = 0;
            local_1a0 = 0;
            if (0xf < local_110) {
              FUN_1800409e0(&local_128,CONCAT71(uStack_127,local_128),local_110 + 1);
            }
            local_110 = 0xf;
            local_118 = 0;
            local_128 = 0;
            if (0xf < local_1e8) {
              FUN_1800409e0(&local_200,CONCAT71(uStack_1ff,local_200),local_1e8 + 1);
            }
            local_1e8 = 0xf;
            local_1f0 = 0;
            local_200 = 0;
            if (0xf < local_f0) {
              FUN_1800409e0(&local_108,CONCAT71(uStack_107,local_108));
            }
            local_f0 = 0xf;
            local_f8 = 0;
            local_108 = 0;
            param_1 = local_288;
          }
          if (*(char *)((longlong)plVar13 + 0x19) == '\0') {
            plVar6 = (longlong *)plVar13[2];
            if (*(char *)((longlong)plVar6 + 0x19) == '\0') {
              cVar1 = *(char *)(*plVar6 + 0x19);
              plVar13 = plVar6;
              plVar6 = (longlong *)*plVar6;
              while (cVar1 == '\0') {
                cVar1 = *(char *)(*plVar6 + 0x19);
                plVar13 = plVar6;
                plVar6 = (longlong *)*plVar6;
              }
            }
            else {
              cVar1 = *(char *)(plVar13[1] + 0x19);
              plVar9 = (longlong *)plVar13[1];
              plVar6 = plVar13;
              while ((plVar13 = plVar9, cVar1 == '\0' && (plVar6 == (longlong *)plVar13[2]))) {
                cVar1 = *(char *)(plVar13[1] + 0x19);
                plVar9 = (longlong *)plVar13[1];
                plVar6 = plVar13;
              }
            }
          }
        }
      }
      FUN_18049bf10(&local_180);
    }
    if (*(char *)((longlong)puVar4 + 0x19) == '\0') {
      puVar7 = (undefined8 *)puVar4[2];
      if (*(char *)((longlong)puVar7 + 0x19) == '\0') {
        cVar1 = *(char *)((longlong)*puVar7 + 0x19);
        puVar4 = puVar7;
        puVar7 = (undefined8 *)*puVar7;
        while (cVar1 == '\0') {
          cVar1 = *(char *)((longlong)*puVar7 + 0x19);
          puVar4 = puVar7;
          puVar7 = (undefined8 *)*puVar7;
        }
      }
      else {
        cVar1 = *(char *)((longlong)puVar4[1] + 0x19);
        puVar10 = (undefined8 *)puVar4[1];
        puVar7 = puVar4;
        while ((puVar4 = puVar10, cVar1 == '\0' && (puVar7 == (undefined8 *)puVar4[2]))) {
          cVar1 = *(char *)((longlong)puVar4[1] + 0x19);
          puVar10 = (undefined8 *)puVar4[1];
          puVar7 = puVar4;
        }
      }
    }
  } while( true );
}



===== 0x180632fc0 180632fc0 FUN_180632fc0 =====

void FUN_180632fc0(longlong param_1)

{
  undefined1 *puVar1;
  longlong *plVar2;
  ulonglong uVar3;
  longlong local_res8 [4];
  
  if (param_1 != 0) {
    local_res8[0] = param_1;
    if (((*(int *)(param_1 + 4) == 0x57) && (*(char *)(param_1 + 0x183) == '\0')) &&
       (*(char *)(param_1 + 0x184) == '\0')) {
      DAT_18135c2e0 = param_1;
      puVar1 = (undefined1 *)FUN_180632580(&DAT_18135c448,local_res8);
      *puVar1 = 0;
      FUN_180633070(local_res8[0]);
    }
    plVar2 = (longlong *)(local_res8[0] + 8);
    uVar3 = 0;
    if (*(longlong *)(local_res8[0] + 0x10) - *plVar2 >> 3 != 0) {
      do {
        FUN_180632fc0(*(undefined8 *)(*plVar2 + uVar3 * 8));
        uVar3 = uVar3 + 1;
        plVar2 = (longlong *)(local_res8[0] + 8);
      } while (uVar3 < (ulonglong)(*(longlong *)(local_res8[0] + 0x10) - *plVar2 >> 3));
    }
  }
  return;
}



===== 0x180633070 180633070 FUN_180633070 =====

void FUN_180633070(longlong param_1)

{
  char cVar1;
  longlong *plVar2;
  undefined8 *puVar3;
  longlong lVar4;
  undefined8 *puVar5;
  undefined8 *puVar6;
  longlong *plVar7;
  longlong *plVar8;
  ulonglong uVar9;
  ulonglong uVar10;
  undefined1 local_res8 [8];
  undefined8 *local_res10;
  undefined8 *local_res18;
  
  if (param_1 != 0) {
    plVar2 = *(longlong **)(param_1 + 0x10);
    plVar7 = *(longlong **)(param_1 + 8);
    if (plVar7 != plVar2) {
      do {
        lVar4 = *plVar7;
        if (*(int *)(lVar4 + 4) == 0x58) {
          plVar8 = *(longlong **)(lVar4 + 8);
          uVar10 = (ulonglong)((longlong)*(longlong **)(lVar4 + 0x10) + (7 - (longlong)plVar8)) >> 3
          ;
          if (*(longlong **)(lVar4 + 0x10) < plVar8) {
            uVar10 = 0;
          }
          uVar9 = 0;
          if (uVar10 != 0) {
            do {
              if (*(int *)(*plVar8 + 4) == 0x89) {
                cVar1 = *(char *)((longlong)DAT_18135c448[1] + 0x19);
                puVar3 = (undefined8 *)DAT_18135c448[1];
                puVar6 = DAT_18135c448;
                while (puVar5 = puVar3, cVar1 == '\0') {
                  if ((ulonglong)puVar5[4] < DAT_18135c2e0) {
                    puVar3 = (undefined8 *)puVar5[2];
                    puVar5 = puVar6;
                  }
                  else {
                    puVar3 = (undefined8 *)*puVar5;
                  }
                  cVar1 = *(char *)((longlong)puVar3 + 0x19);
                  puVar6 = puVar5;
                }
                if ((puVar6 == DAT_18135c448) || (DAT_18135c2e0 < (ulonglong)puVar6[4])) {
                  local_res10 = &DAT_18135c2e0;
                  lVar4 = FUN_180632130(&DAT_18135c448,&DAT_180f064b3,&local_res10,local_res8);
                  FUN_180632190(&DAT_18135c448,&local_res18,puVar6,lVar4 + 0x20,lVar4);
                  puVar6 = local_res18;
                }
                *(undefined1 *)(puVar6 + 5) = 1;
              }
              plVar8 = plVar8 + 1;
              uVar9 = uVar9 + 1;
            } while (uVar9 != uVar10);
          }
        }
        plVar7 = plVar7 + 1;
      } while (plVar7 != plVar2);
    }
  }
  return;
}


