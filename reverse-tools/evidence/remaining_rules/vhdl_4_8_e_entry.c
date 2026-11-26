Target: T-Linter.dll; SHA256: 60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0

===== 0x1806090d0 1806090d0 FUN_1806090d0 =====

/* WARNING: Function: __security_check_cookie replaced with injection: security_check_cookie */
/* WARNING: Removing unreachable block (ram,0x000180609787) */

void FUN_1806090d0(longlong param_1)

{
  undefined1 *puVar1;
  char cVar2;
  undefined4 uVar3;
  undefined8 *puVar4;
  undefined8 *puVar5;
  longlong lVar6;
  undefined8 *puVar7;
  void *pvVar8;
  ulonglong _Size;
  undefined8 *puVar9;
  ulonglong uVar10;
  undefined1 *_Dst;
  undefined1 *puVar11;
  longlong *_Memory;
  void *_Memory_00;
  undefined1 *puVar12;
  undefined1 *puVar13;
  undefined1 auStack_218 [32];
  longlong local_1f8;
  undefined8 local_1f0;
  byte local_1e8;
  undefined7 uStack_1e7;
  undefined8 local_1d8;
  ulonglong local_1d0;
  longlong *local_1c8;
  undefined8 uStack_1c0;
  longlong local_1b8;
  undefined1 local_1b0;
  undefined7 uStack_1af;
  ulonglong local_1a0;
  ulonglong local_198;
  undefined1 local_190;
  undefined7 uStack_18f;
  undefined8 local_180;
  ulonglong local_178;
  undefined1 local_170;
  undefined7 uStack_16f;
  undefined8 local_160;
  ulonglong local_158;
  undefined1 local_150;
  undefined7 uStack_14f;
  undefined8 local_140;
  ulonglong local_138;
  undefined1 local_130;
  undefined7 uStack_12f;
  undefined8 local_120;
  ulonglong local_118;
  undefined1 local_110;
  undefined8 local_100;
  undefined8 local_f8;
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
  
  local_1f0 = 0xfffffffffffffffe;
  local_38 = DAT_18131ef10 ^ (ulonglong)auStack_218;
  puVar4 = *(undefined8 **)(param_1 + 0x78);
  puVar5 = (undefined8 *)*puVar4;
  local_1f8 = param_1;
  do {
    if (puVar5 == puVar4) {
      return;
    }
    uVar10 = 0;
    local_1d0 = 0xf;
    local_1d8 = 0;
    local_1e8 = 0;
    FUN_180040550(&local_1e8,puVar5 + 4,0,0xffffffffffffffff);
    FUN_1801acf00(&local_1c8,puVar5 + 8);
    if (*local_1c8 == 0) {
      if (local_1c8 != (longlong *)0x0) {
        uVar10 = local_1b8 - (longlong)local_1c8 >> 3;
        if (0x1fffffffffffffff < uVar10) {
                    /* WARNING: Subroutine does not return */
          _invalid_parameter_noinfo_noreturn();
        }
        _Memory = local_1c8;
        if (0xfff < uVar10 * 8) {
          if (((ulonglong)local_1c8 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          _Memory = (longlong *)local_1c8[-1];
          if (local_1c8 <= _Memory) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if ((ulonglong)((longlong)local_1c8 - (longlong)_Memory) < 8) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if (0x27 < (ulonglong)((longlong)local_1c8 - (longlong)_Memory)) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
        }
        free(_Memory);
        local_1c8 = (longlong *)0x0;
        uStack_1c0 = 0;
        local_1b8 = 0;
      }
      if (0xf < local_1d0) {
        pvVar8 = (void *)CONCAT71(uStack_1e7,local_1e8);
        _Memory_00 = pvVar8;
        if (0xfff < local_1d0 + 1) {
          if ((local_1e8 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          _Memory_00 = *(void **)((longlong)pvVar8 - 8);
          if (pvVar8 <= _Memory_00) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if ((ulonglong)((longlong)pvVar8 - (longlong)_Memory_00) < 8) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if (0x27 < (ulonglong)((longlong)pvVar8 - (longlong)_Memory_00)) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
        }
        free(_Memory_00);
      }
    }
    else {
      local_198 = 0xf;
      local_1a0 = 0;
      local_1b0 = 0;
      local_118 = 0xf;
      local_120 = 0;
      local_130 = 0;
      local_178 = 0xf;
      local_180 = 0;
      local_190 = 0;
      local_138 = 0xf;
      local_140 = 0;
      local_150 = 0;
      local_158 = 0xf;
      local_160 = 0;
      local_170 = 0;
      local_f8 = 0xf;
      local_100 = 0;
      local_110 = 0;
      lVar6 = *local_1c8;
      DAT_18135a988 = DAT_18135a980;
      FUN_180609b30(lVar6);
      if (DAT_18135a988 - DAT_18135a980 >> 3 != 0) {
        puVar13 = (undefined1 *)(param_1 + 0x10);
        puVar1 = (undefined1 *)(lVar6 + 0x1e0);
        do {
          if (&local_1b0 != puVar13) {
            FUN_180040550(&local_1b0,puVar13,0,0xffffffffffffffff);
          }
          if (&local_190 != puVar1) {
            FUN_180040550(&local_190,puVar1,0,0xffffffffffffffff);
          }
          uVar3 = *(undefined4 *)(*(longlong *)(DAT_18135a980 + uVar10 * 8) + 0x214);
          FUN_180040680(&local_130,&DAT_180f02d20,0x16);
          FUN_180040680(&local_150,"Warning");
          FUN_180040680(&local_170,&DAT_180de47b9);
          _Size = local_1a0;
          local_d0 = 0xf;
          local_d8 = 0;
          local_e8 = 0;
          if (local_1a0 == 0xffffffffffffffff) {
            std::_Xlength_error("string too long");
            return;
          }
          if (local_1a0 < 0x10) {
            if (local_1a0 == 0) {
              local_d8 = local_1a0;
              local_e8 = 0;
              goto LAB_180609424;
            }
LAB_180609429:
            puVar11 = &local_1b0;
            if (0xf < local_198) {
              puVar11 = (undefined1 *)CONCAT71(uStack_1af,local_1b0);
            }
            puVar12 = (undefined1 *)CONCAT71(uStack_e7,local_e8);
            _Dst = &local_e8;
            if (0xf < local_d0) {
              _Dst = puVar12;
            }
            if (_Size != 0) {
              memcpy(_Dst,puVar11,_Size);
              puVar12 = (undefined1 *)CONCAT71(uStack_e7,local_e8);
            }
            local_d8 = _Size;
            puVar11 = &local_e8;
            if (0xf < local_d0) {
              puVar11 = puVar12;
            }
            puVar11[_Size] = 0;
          }
          else {
            FUN_18003f980(&local_e8,local_1a0);
LAB_180609424:
            if (_Size != 0) goto LAB_180609429;
          }
          local_b0 = 0xf;
          local_b8 = 0;
          local_c8[0] = 0;
          FUN_180040550(local_c8,&local_130,0,0xffffffffffffffff);
          local_90 = 0xf;
          local_98 = 0;
          local_a8[0] = 0;
          FUN_180040550(local_a8,&local_190,0);
          local_68 = 0xf;
          local_70 = 0;
          local_80[0] = 0;
          local_88 = uVar3;
          FUN_180040550(local_80,&local_150,0);
          local_48 = 0xf;
          local_50 = 0;
          local_60[0] = 0;
          FUN_180040550(local_60,&local_170,0);
          FUN_18049cca0(&DAT_18136fd88,&local_e8);
          FUN_18049bf90(&local_e8);
          uVar10 = uVar10 + 1;
          param_1 = local_1f8;
        } while (uVar10 < (ulonglong)(DAT_18135a988 - DAT_18135a980 >> 3));
      }
      local_f8 = 0xf;
      local_100 = 0;
      local_110 = 0;
      if (0xf < local_158) {
        FUN_1800409e0(&local_170,CONCAT71(uStack_16f,local_170),local_158 + 1);
      }
      local_158 = 0xf;
      local_160 = 0;
      local_170 = 0;
      if (0xf < local_138) {
        FUN_1800409e0(&local_150,CONCAT71(uStack_14f,local_150),local_138 + 1);
      }
      local_138 = 0xf;
      local_140 = 0;
      local_150 = 0;
      if (0xf < local_178) {
        FUN_1800409e0(&local_190,CONCAT71(uStack_18f,local_190),local_178 + 1);
      }
      local_178 = 0xf;
      local_180 = 0;
      local_190 = 0;
      if (0xf < local_118) {
        FUN_1800409e0(&local_130,CONCAT71(uStack_12f,local_130),local_118 + 1);
      }
      local_118 = 0xf;
      local_120 = 0;
      local_130 = 0;
      if (0xf < local_198) {
        FUN_1800409e0(&local_1b0,CONCAT71(uStack_1af,local_1b0),local_198 + 1);
      }
      local_198 = 0xf;
      local_1a0 = 0;
      local_1b0 = 0;
      if (local_1c8 != (longlong *)0x0) {
        FUN_18043ae30(local_1c8,local_1b8 - (longlong)local_1c8 >> 3);
        local_1c8 = (longlong *)0x0;
        uStack_1c0 = 0;
        local_1b8 = 0;
      }
      if (0xf < local_1d0) {
        FUN_1800409e0(&local_1e8,CONCAT71(uStack_1e7,local_1e8));
      }
    }
    if (*(char *)((longlong)puVar5 + 0x19) == '\0') {
      puVar7 = (undefined8 *)puVar5[2];
      if (*(char *)((longlong)puVar7 + 0x19) == '\0') {
        cVar2 = *(char *)((longlong)*puVar7 + 0x19);
        puVar5 = puVar7;
        puVar7 = (undefined8 *)*puVar7;
        while (cVar2 == '\0') {
          cVar2 = *(char *)((longlong)*puVar7 + 0x19);
          puVar5 = puVar7;
          puVar7 = (undefined8 *)*puVar7;
        }
      }
      else {
        cVar2 = *(char *)((longlong)puVar5[1] + 0x19);
        puVar9 = (undefined8 *)puVar5[1];
        puVar7 = puVar5;
        while ((puVar5 = puVar9, cVar2 == '\0' && (puVar7 == (undefined8 *)puVar5[2]))) {
          cVar2 = *(char *)((longlong)puVar5[1] + 0x19);
          puVar9 = (undefined8 *)puVar5[1];
          puVar7 = puVar5;
        }
      }
    }
  } while( true );
}


