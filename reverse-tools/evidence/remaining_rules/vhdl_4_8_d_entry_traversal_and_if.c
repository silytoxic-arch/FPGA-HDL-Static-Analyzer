Target: T-Linter.dll; SHA256: 60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0

===== 0x18077ad60 18077ad60 FUN_18077ad60 =====

/* WARNING: Function: __security_check_cookie replaced with injection: security_check_cookie */
/* WARNING: Removing unreachable block (ram,0x00018077b04b) */
/* WARNING: Removing unreachable block (ram,0x00018077b091) */

void FUN_18077ad60(longlong param_1)

{
  char cVar1;
  undefined8 *puVar2;
  undefined8 *puVar3;
  undefined8 *puVar4;
  void *pvVar5;
  undefined8 *puVar6;
  ulonglong uVar7;
  longlong *plVar8;
  void *_Memory;
  undefined1 auStack_98 [32];
  undefined8 local_78;
  byte local_70;
  undefined7 uStack_6f;
  undefined8 local_60;
  ulonglong local_58;
  longlong *local_50;
  undefined8 uStack_48;
  longlong local_40;
  ulonglong local_38;
  
  local_78 = 0xfffffffffffffffe;
  local_38 = DAT_18131ef10 ^ (ulonglong)auStack_98;
  puVar2 = *(undefined8 **)(param_1 + 0x78);
  puVar3 = (undefined8 *)*puVar2;
  do {
    if (puVar3 == puVar2) {
      return;
    }
    local_58 = 0xf;
    local_60 = 0;
    local_70 = 0;
    FUN_180040550(&local_70,puVar3 + 4,0,0xffffffffffffffff);
    FUN_1801acf00(&local_50,puVar3 + 8);
    if (*local_50 == 0) {
      if (local_50 != (longlong *)0x0) {
        uVar7 = local_40 - (longlong)local_50 >> 3;
        if (0x1fffffffffffffff < uVar7) {
                    /* WARNING: Subroutine does not return */
          _invalid_parameter_noinfo_noreturn();
        }
        plVar8 = local_50;
        if (0xfff < uVar7 * 8) {
          if (((ulonglong)local_50 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          plVar8 = (longlong *)local_50[-1];
          if (local_50 <= plVar8) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if ((ulonglong)((longlong)local_50 - (longlong)plVar8) < 8) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if (0x27 < (ulonglong)((longlong)local_50 - (longlong)plVar8)) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
        }
        free(plVar8);
        local_50 = (longlong *)0x0;
        uStack_48 = 0;
        local_40 = 0;
      }
      if (0xf < local_58) {
        pvVar5 = (void *)CONCAT71(uStack_6f,local_70);
        _Memory = pvVar5;
        if (0xfff < local_58 + 1) {
          if ((local_70 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          _Memory = *(void **)((longlong)pvVar5 - 8);
          if (pvVar5 <= _Memory) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if ((ulonglong)((longlong)pvVar5 - (longlong)_Memory) < 8) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if (0x27 < (ulonglong)((longlong)pvVar5 - (longlong)_Memory)) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
        }
LAB_18077af97:
        free(_Memory);
      }
    }
    else {
      *(longlong *)(param_1 + 0x178) = *local_50;
      FUN_18077c840(param_1);
      if (local_50 != (longlong *)0x0) {
        uVar7 = local_40 - (longlong)local_50 >> 3;
        if (0x1fffffffffffffff < uVar7) {
                    /* WARNING: Subroutine does not return */
          _invalid_parameter_noinfo_noreturn();
        }
        plVar8 = local_50;
        if (0xfff < uVar7 * 8) {
          if (((ulonglong)local_50 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          plVar8 = (longlong *)local_50[-1];
          if (local_50 <= plVar8) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if ((ulonglong)((longlong)local_50 - (longlong)plVar8) < 8) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if (0x27 < (ulonglong)((longlong)local_50 - (longlong)plVar8)) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
        }
        free(plVar8);
        local_50 = (longlong *)0x0;
        uStack_48 = 0;
        local_40 = 0;
      }
      if (0xf < local_58) {
        pvVar5 = (void *)CONCAT71(uStack_6f,local_70);
        _Memory = pvVar5;
        if (0xfff < local_58 + 1) {
          if ((local_70 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          _Memory = *(void **)((longlong)pvVar5 - 8);
          if (pvVar5 <= _Memory) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if ((ulonglong)((longlong)pvVar5 - (longlong)_Memory) < 8) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
          if (0x27 < (ulonglong)((longlong)pvVar5 - (longlong)_Memory)) {
                    /* WARNING: Subroutine does not return */
            _invalid_parameter_noinfo_noreturn();
          }
        }
        goto LAB_18077af97;
      }
    }
    if (*(char *)((longlong)puVar3 + 0x19) == '\0') {
      puVar4 = (undefined8 *)puVar3[2];
      if (*(char *)((longlong)puVar4 + 0x19) == '\0') {
        cVar1 = *(char *)((longlong)*puVar4 + 0x19);
        puVar3 = puVar4;
        puVar4 = (undefined8 *)*puVar4;
        while (cVar1 == '\0') {
          cVar1 = *(char *)((longlong)*puVar4 + 0x19);
          puVar3 = puVar4;
          puVar4 = (undefined8 *)*puVar4;
        }
      }
      else {
        cVar1 = *(char *)((longlong)puVar3[1] + 0x19);
        puVar6 = (undefined8 *)puVar3[1];
        puVar4 = puVar3;
        while ((puVar3 = puVar6, cVar1 == '\0' && (puVar4 == (undefined8 *)puVar3[2]))) {
          cVar1 = *(char *)((longlong)puVar3[1] + 0x19);
          puVar6 = (undefined8 *)puVar3[1];
          puVar4 = puVar3;
        }
      }
    }
  } while( true );
}



===== 0x18077c840 18077c840 FUN_18077c840 =====

/* WARNING: Function: __security_check_cookie replaced with injection: security_check_cookie */
/* WARNING: Removing unreachable block (ram,0x00018077caa2) */
/* WARNING: Removing unreachable block (ram,0x00018077caa7) */
/* WARNING: Removing unreachable block (ram,0x00018077cab0) */
/* WARNING: Removing unreachable block (ram,0x00018077cade) */
/* WARNING: Removing unreachable block (ram,0x00018077cae8) */
/* WARNING: Removing unreachable block (ram,0x00018077caee) */
/* WARNING: Removing unreachable block (ram,0x00018077cafb) */
/* WARNING: Removing unreachable block (ram,0x00018077caba) */
/* WARNING: Removing unreachable block (ram,0x00018077cac6) */
/* WARNING: Removing unreachable block (ram,0x00018077cad0) */
/* WARNING: Removing unreachable block (ram,0x00018077cadc) */
/* WARNING: Removing unreachable block (ram,0x00018077cafe) */

void FUN_18077c840(longlong param_1,longlong param_2)

{
  char cVar1;
  longlong *plVar2;
  longlong *plVar3;
  longlong *plVar4;
  undefined8 uVar5;
  undefined8 *puVar6;
  longlong *plVar7;
  ulonglong *puVar8;
  ulonglong uVar9;
  ulonglong uVar10;
  undefined1 auStack_f8 [32];
  char local_d8;
  char local_d7;
  ulonglong local_d0;
  longlong *local_c8;
  undefined8 local_c0;
  longlong local_b8;
  longlong local_b0;
  undefined8 local_a8;
  int local_98;
  undefined1 local_90 [24];
  undefined8 local_78 [10];
  ulonglong local_28;
  
  local_a8 = 0xfffffffffffffffe;
  local_28 = DAT_18131ef10 ^ (ulonglong)auStack_f8;
  local_d0 = param_1;
  local_b8 = param_2;
  if (*(int *)(param_2 + 4) != 0x52) {
    if (((*(char *)(param_2 + 0x183) != '\0') && (*(longlong *)(param_2 + 0x188) != 0)) &&
       (*(char *)(param_1 + 0x15f) == '\0')) {
      FUN_18077b8e0();
    }
    if ((*(int *)(param_2 + 4) == 0x56) && (*(char *)(param_1 + 0x15f) == '\0')) {
      FUN_18077c530(param_1,param_2);
    }
    if (((*(int *)(param_2 + 4) == 0x57) && (*(char *)(param_2 + 0x183) == '\0')) &&
       ((*(char *)(param_2 + 0x184) == '\0' &&
        ((1 < (ulonglong)(*(longlong *)(param_2 + 0x10) - *(longlong *)(param_2 + 8) >> 3) &&
         (*(char *)(param_1 + 0x15f) == '\0')))))) {
      FUN_18077b0c0(param_1,param_2);
    }
    puVar6 = *(undefined8 **)(param_2 + 8);
    local_d0 = 0;
    uVar9 = (ulonglong)((longlong)*(undefined8 **)(param_2 + 0x10) + (7 - (longlong)puVar6)) >> 3;
    if (*(undefined8 **)(param_2 + 0x10) < puVar6) {
      uVar9 = 0;
    }
    if (uVar9 == 0) {
      return;
    }
    do {
      FUN_18077c840(param_1,*puVar6);
      puVar6 = puVar6 + 1;
      local_d0 = local_d0 + 1;
    } while (local_d0 != uVar9);
    return;
  }
  *(undefined1 *)(param_1 + 0x15f) = 0;
  puVar8 = (ulonglong *)(param_1 + 0x160);
  *(ulonglong *)(param_1 + 0x168) = *puVar8;
  local_c8 = (longlong *)0x0;
  local_c0 = 0;
  local_c8 = (longlong *)FUN_1804bb210(&local_c8);
  FUN_1804d56f0(param_2,&local_c8,*(undefined8 *)(local_d0 + 0x178));
  plVar3 = local_c8;
  local_d8 = '\x01';
  local_d7 = '\0';
  plVar7 = (longlong *)*local_c8;
  if (plVar7 != local_c8) {
    do {
      local_98 = (int)plVar7[4];
      FUN_1804c1810(local_90);
      if (local_98 == 1) {
        local_d8 = '\0';
        puVar6 = *(undefined8 **)(param_1 + 0x168);
        if ((local_78 < puVar6) && ((undefined8 *)*puVar8 <= local_78)) {
          local_b0 = (longlong)local_78 - (longlong)*puVar8 >> 3;
          if (puVar6 == *(undefined8 **)(param_1 + 0x170)) {
            FUN_1800db6d0(puVar8);
          }
          puVar6 = *(undefined8 **)(param_1 + 0x168);
          if (puVar6 != (undefined8 *)0x0) {
            uVar5 = *(undefined8 *)(*puVar8 + local_b0 * 8);
LAB_18077c97a:
            *puVar6 = uVar5;
          }
        }
        else {
          if (puVar6 == *(undefined8 **)(param_1 + 0x170)) {
            FUN_1800db6d0(puVar8);
          }
          puVar6 = *(undefined8 **)(param_1 + 0x168);
          uVar5 = local_78[0];
          if (puVar6 != (undefined8 *)0x0) goto LAB_18077c97a;
        }
        *(longlong *)(param_1 + 0x168) = *(longlong *)(param_1 + 0x168) + 8;
LAB_18077c996:
        *(undefined1 *)(local_d0 + 0x15f) = 1;
      }
      else if ((local_98 - 2U & 0xfffffffd) == 0) {
        local_d8 = '\0';
        local_d7 = '\x01';
        goto LAB_18077c996;
      }
      FUN_18019aa70(local_90);
      if (*(char *)((longlong)plVar7 + 0x19) == '\0') {
        plVar2 = (longlong *)plVar7[2];
        if (*(char *)((longlong)plVar2 + 0x19) == '\0') {
          cVar1 = *(char *)(*plVar2 + 0x19);
          plVar7 = plVar2;
          plVar2 = (longlong *)*plVar2;
          while (cVar1 == '\0') {
            cVar1 = *(char *)(*plVar2 + 0x19);
            plVar7 = plVar2;
            plVar2 = (longlong *)*plVar2;
          }
        }
        else {
          cVar1 = *(char *)(plVar7[1] + 0x19);
          plVar4 = (longlong *)plVar7[1];
          plVar2 = plVar7;
          while ((plVar7 = plVar4, cVar1 == '\0' && (plVar2 == (longlong *)plVar7[2]))) {
            cVar1 = *(char *)(plVar7[1] + 0x19);
            plVar4 = (longlong *)plVar7[1];
            plVar2 = plVar7;
          }
        }
      }
    } while (plVar7 != plVar3);
    if ((local_d7 != '\0') || (local_d8 == '\0')) goto LAB_18077ca68;
  }
  *(undefined1 *)(local_d0 + 0x15f) = 0;
  puVar6 = *(undefined8 **)(local_b8 + 8);
  uVar10 = 0;
  uVar9 = (ulonglong)((longlong)*(undefined8 **)(local_b8 + 0x10) + (7 - (longlong)puVar6)) >> 3;
  if (*(undefined8 **)(local_b8 + 0x10) < puVar6) {
    uVar9 = uVar10;
  }
  if (uVar9 != 0) {
    do {
      FUN_18077c840(local_d0,*puVar6);
      puVar6 = puVar6 + 1;
      uVar10 = uVar10 + 1;
    } while (uVar10 != uVar9);
  }
LAB_18077ca68:
  FUN_1804bb2b0(&local_c8,local_c8[1]);
  local_c8[1] = (longlong)local_c8;
  *local_c8 = (longlong)local_c8;
  local_c8[2] = (longlong)local_c8;
  local_c0 = 0;
  free(local_c8);
  return;
}



===== 0x18077b8e0 18077b8e0 FUN_18077b8e0 =====

/* WARNING: Function: __security_check_cookie replaced with injection: security_check_cookie */
/* WARNING: Removing unreachable block (ram,0x00018077c3c9) */
/* WARNING: Removing unreachable block (ram,0x00018077bd33) */
/* WARNING: Removing unreachable block (ram,0x00018077bd38) */
/* WARNING: Removing unreachable block (ram,0x00018077bd40) */
/* WARNING: Removing unreachable block (ram,0x00018077bd49) */
/* WARNING: Removing unreachable block (ram,0x00018077bd6e) */
/* WARNING: Removing unreachable block (ram,0x00018077bd78) */
/* WARNING: Removing unreachable block (ram,0x00018077bd7e) */
/* WARNING: Removing unreachable block (ram,0x00018077bd8b) */
/* WARNING: Removing unreachable block (ram,0x00018077bd53) */
/* WARNING: Removing unreachable block (ram,0x00018077bd60) */
/* WARNING: Removing unreachable block (ram,0x00018077bd6c) */
/* WARNING: Removing unreachable block (ram,0x00018077bd8e) */
/* WARNING: Removing unreachable block (ram,0x00018077c3a6) */

void FUN_18077b8e0(longlong param_1,longlong param_2)

{
  char cVar1;
  int iVar2;
  longlong lVar3;
  longlong *plVar4;
  undefined8 *puVar5;
  void *pvVar6;
  longlong *plVar7;
  undefined8 *puVar8;
  longlong *plVar9;
  undefined8 *puVar10;
  void *pvVar11;
  undefined8 *puVar12;
  ulonglong uVar13;
  uint uVar14;
  longlong lVar15;
  byte *pbVar16;
  undefined1 *puVar17;
  ulonglong uVar18;
  undefined1 auStack_248 [32];
  uint local_228;
  undefined8 *local_220;
  undefined8 local_218;
  undefined8 *local_210;
  undefined8 local_208;
  undefined1 local_200 [16];
  undefined8 local_1f0;
  undefined8 local_1e8;
  undefined1 local_1e0 [16];
  undefined8 local_1d0;
  undefined8 local_1c8;
  undefined1 local_1c0 [16];
  undefined8 local_1b0;
  undefined8 local_1a8;
  undefined8 *local_1a0;
  undefined8 local_198;
  undefined8 *local_190;
  undefined8 local_188;
  undefined1 *local_180;
  undefined1 *local_178;
  undefined1 *local_170;
  undefined1 *local_168;
  undefined1 *local_160;
  undefined1 *local_158;
  longlong *local_150;
  longlong *local_148;
  longlong *local_140;
  longlong *local_138;
  longlong *local_130;
  longlong *local_128;
  undefined8 local_120;
  undefined1 local_118;
  undefined7 uStack_117;
  undefined8 local_108;
  ulonglong local_100;
  byte local_f8;
  undefined7 uStack_f7;
  undefined8 local_e8;
  ulonglong local_e0;
  undefined8 local_d8;
  undefined1 local_d0;
  undefined7 uStack_cf;
  undefined8 local_c0;
  ulonglong local_b8;
  undefined8 local_b0;
  undefined1 local_a8;
  undefined7 uStack_a7;
  undefined8 local_98;
  ulonglong local_90;
  undefined8 local_88;
  byte local_80;
  undefined7 uStack_7f;
  undefined8 local_70;
  ulonglong local_68;
  undefined1 local_60;
  undefined7 uStack_5f;
  undefined8 local_50;
  ulonglong local_48;
  ulonglong local_40;
  
  local_120 = 0xfffffffffffffffe;
  local_40 = DAT_18131ef10 ^ (ulonglong)auStack_248;
  uVar18 = 0;
  local_210 = (undefined8 *)0x0;
  local_208 = 0;
  local_210 = (undefined8 *)FUN_1800da120(&local_210);
  uVar13 = uVar18;
  if (*(longlong *)(param_2 + 0x10) - *(longlong *)(param_2 + 8) >> 3 != 0) {
    do {
      lVar3 = *(longlong *)(*(longlong *)(param_2 + 8) + uVar13 * 8);
      iVar2 = *(int *)(lVar3 + 4);
      if (((iVar2 == 0x58) || (iVar2 == 0xab)) || (iVar2 == 0xac)) {
        FUN_18077cc00(param_1,lVar3,&local_210);
      }
      uVar13 = uVar13 + 1;
    } while (uVar13 < (ulonglong)(*(longlong *)(param_2 + 0x10) - *(longlong *)(param_2 + 8) >> 3));
  }
  uVar14 = 0;
  if (*(longlong *)(param_2 + 0x10) - *(longlong *)(param_2 + 8) >> 3 != 0) {
    do {
      lVar3 = *(longlong *)(*(longlong *)(param_2 + 8) + uVar18 * 8);
      iVar2 = *(int *)(lVar3 + 4);
      local_228 = uVar14 & 0xff;
      if (iVar2 == 0xac) {
        local_228 = 1;
      }
      if (iVar2 == 0x58) {
        local_220 = (undefined8 *)0x0;
        local_218 = 0;
        local_220 = (undefined8 *)FUN_1800da120(&local_220);
        FUN_18077cc00(param_1,lVar3,&local_220);
        puVar8 = local_210;
        puVar12 = (undefined8 *)*local_210;
        while (puVar12 != puVar8) {
          local_e0 = 0xf;
          local_e8 = 0;
          local_f8 = 0;
          FUN_180040550(&local_f8,puVar12 + 4,0,0xffffffffffffffff);
          local_d8 = puVar12[8];
          if (*(char *)(param_1 + 0x159) != '\0') {
            FUN_18093fd20(&local_220,&local_150,&local_f8);
            lVar15 = 0;
            plVar7 = local_150;
            while (plVar7 != local_148) {
              lVar15 = lVar15 + 1;
              if (*(char *)((longlong)plVar7 + 0x19) == '\0') {
                plVar4 = (longlong *)plVar7[2];
                if (*(char *)((longlong)plVar4 + 0x19) == '\0') {
                  cVar1 = *(char *)(*plVar4 + 0x19);
                  plVar7 = plVar4;
                  plVar4 = (longlong *)*plVar4;
                  while (cVar1 == '\0') {
                    cVar1 = *(char *)(*plVar4 + 0x19);
                    plVar7 = plVar4;
                    plVar4 = (longlong *)*plVar4;
                  }
                }
                else {
                  cVar1 = *(char *)(plVar7[1] + 0x19);
                  plVar9 = (longlong *)plVar7[1];
                  plVar4 = plVar7;
                  while ((plVar7 = plVar9, cVar1 == '\0' && (plVar4 == (longlong *)plVar7[2]))) {
                    cVar1 = *(char *)(plVar7[1] + 0x19);
                    plVar9 = (longlong *)plVar7[1];
                    plVar4 = plVar7;
                  }
                }
              }
            }
            if (lVar15 == 0) {
              pbVar16 = &local_f8;
              if (0xf < local_e0) {
                pbVar16 = (byte *)CONCAT71(uStack_f7,local_f8);
              }
              FUN_1809ca4f0(&local_80,&DAT_180f1d5f0,pbVar16);
              local_180 = local_200;
              local_178 = local_1e0;
              local_170 = local_1c0;
              local_1e8 = 0xf;
              local_1f0 = 0;
              local_200[0] = 0;
              FUN_180040550(local_200,lVar3 + 0x1e0,0,0xffffffffffffffff);
              local_1c8 = 0xf;
              local_1d0 = 0;
              local_1e0[0] = 0;
              FUN_180040550(local_1e0,&local_80,0,0xffffffffffffffff);
              local_1a8 = 0xf;
              local_1b0 = 0;
              local_1c0[0] = 0;
              FUN_180040550(local_1c0,param_1 + 0x10,0,0xffffffffffffffff);
              FUN_18077cd10(local_1c0,local_1e0);
              if (0xf < local_68) {
                pvVar6 = (void *)CONCAT71(uStack_7f,local_80);
                pvVar11 = pvVar6;
                if (0xfff < local_68 + 1) {
                  if ((local_80 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
                    _invalid_parameter_noinfo_noreturn();
                  }
                  pvVar11 = *(void **)((longlong)pvVar6 - 8);
                  if (pvVar6 <= pvVar11) {
                    /* WARNING: Subroutine does not return */
                    _invalid_parameter_noinfo_noreturn();
                  }
                  if ((ulonglong)((longlong)pvVar6 - (longlong)pvVar11) < 8) {
                    /* WARNING: Subroutine does not return */
                    _invalid_parameter_noinfo_noreturn();
                  }
                  if (0x27 < (ulonglong)((longlong)pvVar6 - (longlong)pvVar11)) {
                    /* WARNING: Subroutine does not return */
                    _invalid_parameter_noinfo_noreturn();
                  }
                }
                free(pvVar11);
              }
              local_68 = 0xf;
              local_70 = 0;
              local_80 = 0;
            }
          }
          if (0xf < local_e0) {
            pvVar6 = (void *)CONCAT71(uStack_f7,local_f8);
            pvVar11 = pvVar6;
            if (0xfff < local_e0 + 1) {
              if ((local_f8 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
                _invalid_parameter_noinfo_noreturn();
              }
              pvVar11 = *(void **)((longlong)pvVar6 - 8);
              if (pvVar6 <= pvVar11) {
                    /* WARNING: Subroutine does not return */
                _invalid_parameter_noinfo_noreturn();
              }
              if ((ulonglong)((longlong)pvVar6 - (longlong)pvVar11) < 8) {
                    /* WARNING: Subroutine does not return */
                _invalid_parameter_noinfo_noreturn();
              }
              if (0x27 < (ulonglong)((longlong)pvVar6 - (longlong)pvVar11)) {
                    /* WARNING: Subroutine does not return */
                _invalid_parameter_noinfo_noreturn();
              }
            }
            free(pvVar11);
          }
          local_e0 = 0xf;
          local_e8 = 0;
          local_f8 = 0;
          if (*(char *)((longlong)puVar12 + 0x19) == '\0') {
            puVar5 = (undefined8 *)puVar12[2];
            if (*(char *)((longlong)puVar5 + 0x19) == '\0') {
              cVar1 = *(char *)((longlong)*puVar5 + 0x19);
              puVar12 = puVar5;
              puVar5 = (undefined8 *)*puVar5;
              while (cVar1 == '\0') {
                cVar1 = *(char *)((longlong)*puVar5 + 0x19);
                puVar12 = puVar5;
                puVar5 = (undefined8 *)*puVar5;
              }
            }
            else {
              cVar1 = *(char *)((longlong)puVar12[1] + 0x19);
              puVar10 = (undefined8 *)puVar12[1];
              puVar5 = puVar12;
              while ((puVar12 = puVar10, cVar1 == '\0' && (puVar5 == (undefined8 *)puVar12[2]))) {
                cVar1 = *(char *)((longlong)puVar12[1] + 0x19);
                puVar10 = (undefined8 *)puVar12[1];
                puVar5 = puVar12;
              }
            }
          }
        }
        FUN_1800dab10(&local_220,local_220[1]);
        local_220[1] = local_220;
        *local_220 = local_220;
        local_220[2] = local_220;
        local_218 = 0;
        puVar12 = local_220;
LAB_18077c363:
        free(puVar12);
      }
      else {
        if (iVar2 == 0xab) {
          local_1a0 = (undefined8 *)0x0;
          local_198 = 0;
          local_1a0 = (undefined8 *)FUN_1800da120(&local_1a0);
          FUN_18077cc00(param_1,lVar3,&local_1a0);
          puVar8 = local_210;
          puVar12 = (undefined8 *)*local_210;
          while (puVar12 != puVar8) {
            local_b8 = 0xf;
            local_c0 = 0;
            local_d0 = 0;
            FUN_180040550(&local_d0,puVar12 + 4,0,0xffffffffffffffff);
            local_b0 = puVar12[8];
            if (*(char *)(param_1 + 0x159) != '\0') {
              FUN_18093fd20(&local_1a0,&local_140,&local_d0);
              lVar15 = 0;
              plVar7 = local_140;
              while (plVar7 != local_138) {
                lVar15 = lVar15 + 1;
                if (*(char *)((longlong)plVar7 + 0x19) == '\0') {
                  plVar4 = (longlong *)plVar7[2];
                  if (*(char *)((longlong)plVar4 + 0x19) == '\0') {
                    cVar1 = *(char *)(*plVar4 + 0x19);
                    plVar7 = plVar4;
                    plVar4 = (longlong *)*plVar4;
                    while (cVar1 == '\0') {
                      cVar1 = *(char *)(*plVar4 + 0x19);
                      plVar7 = plVar4;
                      plVar4 = (longlong *)*plVar4;
                    }
                  }
                  else {
                    cVar1 = *(char *)(plVar7[1] + 0x19);
                    plVar9 = (longlong *)plVar7[1];
                    plVar4 = plVar7;
                    while ((plVar7 = plVar9, cVar1 == '\0' && (plVar4 == (longlong *)plVar7[2]))) {
                      cVar1 = *(char *)(plVar7[1] + 0x19);
                      plVar9 = (longlong *)plVar7[1];
                      plVar4 = plVar7;
                    }
                  }
                }
              }
              if (lVar15 == 0) {
                puVar17 = &local_d0;
                if (0xf < local_b8) {
                  puVar17 = (undefined1 *)CONCAT71(uStack_cf,local_d0);
                }
                FUN_1809ca4f0(&local_60,&DAT_180f1d628,puVar17);
                local_170 = local_1c0;
                local_178 = local_1e0;
                local_180 = local_200;
                local_1a8 = 0xf;
                local_1b0 = 0;
                local_1c0[0] = 0;
                FUN_180040550(local_1c0,lVar3 + 0x1e0,0,0xffffffffffffffff);
                local_1c8 = 0xf;
                local_1d0 = 0;
                local_1e0[0] = 0;
                FUN_180040550(local_1e0,&local_60,0,0xffffffffffffffff);
                local_1e8 = 0xf;
                local_1f0 = 0;
                local_200[0] = 0;
                FUN_180040550(local_200,param_1 + 0x10,0,0xffffffffffffffff);
                FUN_18077cd10(local_200,local_1e0,local_1c0);
                if (0xf < local_48) {
                  FUN_1800409e0(&local_60,CONCAT71(uStack_5f,local_60),local_48 + 1);
                }
                local_48 = 0xf;
                local_50 = 0;
                local_60 = 0;
              }
            }
            if (0xf < local_b8) {
              FUN_1800409e0(&local_d0,CONCAT71(uStack_cf,local_d0));
            }
            local_b8 = 0xf;
            local_c0 = 0;
            local_d0 = 0;
            if (*(char *)((longlong)puVar12 + 0x19) == '\0') {
              puVar5 = (undefined8 *)puVar12[2];
              if (*(char *)((longlong)puVar5 + 0x19) == '\0') {
                cVar1 = *(char *)((longlong)*puVar5 + 0x19);
                puVar12 = puVar5;
                puVar5 = (undefined8 *)*puVar5;
                while (cVar1 == '\0') {
                  cVar1 = *(char *)((longlong)*puVar5 + 0x19);
                  puVar12 = puVar5;
                  puVar5 = (undefined8 *)*puVar5;
                }
              }
              else {
                cVar1 = *(char *)((longlong)puVar12[1] + 0x19);
                puVar10 = (undefined8 *)puVar12[1];
                puVar5 = puVar12;
                while ((puVar12 = puVar10, cVar1 == '\0' && (puVar5 == (undefined8 *)puVar12[2]))) {
                  cVar1 = *(char *)((longlong)puVar12[1] + 0x19);
                  puVar10 = (undefined8 *)puVar12[1];
                  puVar5 = puVar12;
                }
              }
            }
          }
          FUN_1800eb720(&local_1a0,&local_158,*local_1a0);
          puVar12 = local_1a0;
          goto LAB_18077c363;
        }
        if (iVar2 == 0xac) {
          local_190 = (undefined8 *)0x0;
          local_188 = 0;
          local_190 = (undefined8 *)FUN_1800da120(&local_190);
          FUN_18077cc00(param_1,lVar3,&local_190);
          puVar8 = local_210;
          puVar12 = (undefined8 *)*local_210;
          while (puVar12 != puVar8) {
            local_90 = 0xf;
            local_98 = 0;
            local_a8 = 0;
            FUN_180040550(&local_a8,puVar12 + 4,0,0xffffffffffffffff);
            local_88 = puVar12[8];
            if (*(char *)(param_1 + 0x159) != '\0') {
              FUN_18093fd20(&local_190,&local_130,&local_a8);
              lVar15 = 0;
              plVar7 = local_130;
              while (plVar7 != local_128) {
                lVar15 = lVar15 + 1;
                if (*(char *)((longlong)plVar7 + 0x19) == '\0') {
                  plVar4 = (longlong *)plVar7[2];
                  if (*(char *)((longlong)plVar4 + 0x19) == '\0') {
                    cVar1 = *(char *)(*plVar4 + 0x19);
                    plVar7 = plVar4;
                    plVar4 = (longlong *)*plVar4;
                    while (cVar1 == '\0') {
                      cVar1 = *(char *)(*plVar4 + 0x19);
                      plVar7 = plVar4;
                      plVar4 = (longlong *)*plVar4;
                    }
                  }
                  else {
                    cVar1 = *(char *)(plVar7[1] + 0x19);
                    plVar9 = (longlong *)plVar7[1];
                    plVar4 = plVar7;
                    while ((plVar7 = plVar9, cVar1 == '\0' && (plVar4 == (longlong *)plVar7[2]))) {
                      cVar1 = *(char *)(plVar7[1] + 0x19);
                      plVar9 = (longlong *)plVar7[1];
                      plVar4 = plVar7;
                    }
                  }
                }
              }
              if (lVar15 == 0) {
                puVar17 = &local_a8;
                if (0xf < local_90) {
                  puVar17 = (undefined1 *)CONCAT71(uStack_a7,local_a8);
                }
                FUN_1809ca4f0(&local_118,&DAT_180f1d660,puVar17);
                local_170 = local_1c0;
                local_178 = local_1e0;
                local_180 = local_200;
                local_1a8 = 0xf;
                local_1b0 = 0;
                local_1c0[0] = 0;
                FUN_180040550(local_1c0,lVar3 + 0x1e0,0,0xffffffffffffffff);
                local_1c8 = 0xf;
                local_1d0 = 0;
                local_1e0[0] = 0;
                FUN_180040550(local_1e0,&local_118,0,0xffffffffffffffff);
                local_1e8 = 0xf;
                local_1f0 = 0;
                local_200[0] = 0;
                FUN_180040550(local_200,param_1 + 0x10,0,0xffffffffffffffff);
                FUN_18077cd10(local_200,local_1e0,local_1c0);
                if (0xf < local_100) {
                  FUN_1800409e0(&local_118,CONCAT71(uStack_117,local_118),local_100 + 1);
                }
                local_100 = 0xf;
                local_108 = 0;
                local_118 = 0;
              }
            }
            if (0xf < local_90) {
              FUN_1800409e0(&local_a8,CONCAT71(uStack_a7,local_a8));
            }
            local_90 = 0xf;
            local_98 = 0;
            local_a8 = 0;
            if (*(char *)((longlong)puVar12 + 0x19) == '\0') {
              puVar5 = (undefined8 *)puVar12[2];
              if (*(char *)((longlong)puVar5 + 0x19) == '\0') {
                cVar1 = *(char *)((longlong)*puVar5 + 0x19);
                puVar12 = puVar5;
                puVar5 = (undefined8 *)*puVar5;
                while (cVar1 == '\0') {
                  cVar1 = *(char *)((longlong)*puVar5 + 0x19);
                  puVar12 = puVar5;
                  puVar5 = (undefined8 *)*puVar5;
                }
              }
              else {
                cVar1 = *(char *)((longlong)puVar12[1] + 0x19);
                puVar10 = (undefined8 *)puVar12[1];
                puVar5 = puVar12;
                while ((puVar12 = puVar10, cVar1 == '\0' && (puVar5 == (undefined8 *)puVar12[2]))) {
                  cVar1 = *(char *)((longlong)puVar12[1] + 0x19);
                  puVar10 = (undefined8 *)puVar12[1];
                  puVar5 = puVar12;
                }
              }
            }
          }
          FUN_1800eb720(&local_190,&local_168,*local_190);
          puVar12 = local_190;
          goto LAB_18077c363;
        }
      }
      uVar18 = uVar18 + 1;
      uVar14 = local_228;
    } while (uVar18 < (ulonglong)(*(longlong *)(param_2 + 0x10) - *(longlong *)(param_2 + 8) >> 3));
    if ((char)local_228 != '\0') goto LAB_18077c4d0;
  }
  if (*(char *)(param_1 + 0x158) == '\x01') {
    FUN_1809ca4f0(&local_118,&DAT_180f1d698);
    local_168 = local_1c0;
    local_158 = local_1e0;
    local_160 = local_200;
    local_1a8 = 0xf;
    local_1b0 = 0;
    local_1c0[0] = 0;
    FUN_180040550(local_1c0,param_2 + 0x1e0,0,0xffffffffffffffff);
    local_1c8 = 0xf;
    local_1d0 = 0;
    local_1e0[0] = 0;
    FUN_180040550(local_1e0,&local_118,0,0xffffffffffffffff);
    local_1e8 = 0xf;
    local_1f0 = 0;
    local_200[0] = 0;
    FUN_180040550(local_200,param_1 + 0x10,0,0xffffffffffffffff);
    FUN_18077cd10(local_200,local_1e0,local_1c0,*(undefined4 *)(param_2 + 0x214));
    if (0xf < local_100) {
      FUN_1800409e0(&local_118,CONCAT71(uStack_117,local_118),local_100 + 1);
    }
    local_100 = 0xf;
    local_108 = 0;
    local_118 = 0;
  }
LAB_18077c4d0:
  FUN_1800eb720(&local_210,&local_168,*local_210,local_210);
  FUN_180309c90(local_210,1);
  return;
}


