Target: T-Linter.dll; SHA256: 60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0

===== 0x1800dd7e0 1800dd7e0 FUN_1800dd7e0 =====

void FUN_1800dd7e0(longlong param_1)

{
  ulonglong *puVar1;
  longlong lVar2;
  void **ppvVar3;
  void **ppvVar4;
  void *pvVar5;
  undefined8 uVar6;
  void *local_res8;
  undefined8 uVar7;
  
  uVar7 = 0xfffffffffffffffe;
  *(undefined1 *)(param_1 + 0x183) = 1;
  local_res8 = operator_new(0x2c8);
  if (local_res8 == (void *)0x0) {
    pvVar5 = (void *)0x0;
  }
  else {
    if ((*(longlong *)(param_1 + 0x98) == *(longlong *)(param_1 + 0x90)) ||
       (*(char *)(*(longlong *)(param_1 + 0x98) + -1) != 'x')) {
      if ((*(longlong *)(param_1 + 0x98) == *(longlong *)(param_1 + 0x90)) ||
         (uVar6 = 0x5a, *(char *)(*(longlong *)(param_1 + 0x98) + -1) != 'z')) {
        uVar6 = 0x58;
      }
    }
    else {
      uVar6 = 0x59;
    }
    pvVar5 = (void *)FUN_1808d8dc0(local_res8,uVar6,0,0,0,0,uVar7);
  }
  lVar2 = *(longlong *)(*(longlong *)(param_1 + 0x10) + -8);
  ppvVar3 = *(void ***)(lVar2 + 0x10);
  local_res8 = pvVar5;
  if ((&local_res8 < ppvVar3) && (ppvVar4 = *(void ***)(lVar2 + 8), ppvVar4 <= &local_res8)) {
    if (ppvVar3 == *(void ***)(lVar2 + 0x18)) {
      FUN_1800db6d0(lVar2 + 8,1);
    }
    if (*(undefined8 **)(lVar2 + 0x10) != (undefined8 *)0x0) {
      **(undefined8 **)(lVar2 + 0x10) =
           *(undefined8 *)
            (*(longlong *)(lVar2 + 8) + ((longlong)&local_res8 - (longlong)ppvVar4 >> 3) * 8);
    }
  }
  else {
    if (ppvVar3 == *(void ***)(lVar2 + 0x18)) {
      FUN_1800db6d0(lVar2 + 8,1);
    }
    if (*(undefined8 **)(lVar2 + 0x10) != (undefined8 *)0x0) {
      **(undefined8 **)(lVar2 + 0x10) = pvVar5;
    }
  }
  *(longlong *)(lVar2 + 0x10) = *(longlong *)(lVar2 + 0x10) + 8;
  puVar1 = (ulonglong *)(param_1 + 8);
  ppvVar3 = *(void ***)(param_1 + 0x10);
  if ((&local_res8 < ppvVar3) && (ppvVar4 = (void **)*puVar1, ppvVar4 <= &local_res8)) {
    if (ppvVar3 == *(void ***)(param_1 + 0x18)) {
      FUN_1800db6d0(puVar1,1);
    }
    if (*(undefined8 **)(param_1 + 0x10) != (undefined8 *)0x0) {
      **(undefined8 **)(param_1 + 0x10) =
           *(undefined8 *)(*puVar1 + ((longlong)&local_res8 - (longlong)ppvVar4 >> 3) * 8);
    }
  }
  else {
    if (ppvVar3 == *(void ***)(param_1 + 0x18)) {
      FUN_1800db6d0(puVar1,1);
    }
    if (*(undefined8 **)(param_1 + 0x10) != (undefined8 *)0x0) {
      **(undefined8 **)(param_1 + 0x10) = pvVar5;
    }
  }
  *(longlong *)(param_1 + 0x10) = *(longlong *)(param_1 + 0x10) + 8;
  return;
}



===== 0x1800de4a0 1800de4a0 FUN_1800de4a0 =====

/* WARNING: Function: __security_check_cookie replaced with injection: security_check_cookie */

void FUN_1800de4a0(longlong param_1,longlong *param_2)

{
  char cVar1;
  ulonglong ****ppppuVar2;
  ulonglong ***pppuVar3;
  ulonglong ***pppuVar4;
  int iVar5;
  undefined4 uVar6;
  int iVar7;
  uint uVar8;
  longlong *plVar9;
  longlong *plVar10;
  longlong *plVar11;
  longlong *plVar12;
  longlong *plVar13;
  longlong lVar14;
  ulonglong ****ppppuVar15;
  ulonglong ****ppppuVar16;
  void *pvVar17;
  undefined8 uVar18;
  ulonglong ****ppppuVar19;
  ulonglong **ppuVar20;
  ulonglong ****ppppuVar21;
  ulonglong uVar22;
  ulonglong **ppuVar23;
  undefined8 *puVar24;
  ulonglong **ppuVar25;
  ulonglong **ppuVar26;
  ulonglong ****_Size;
  undefined1 auStack_d8 [32];
  longlong local_b8;
  undefined8 local_b0;
  undefined1 local_a8 [8];
  ulonglong ***local_a0;
  ulonglong ***local_98;
  ulonglong ***local_90;
  ulonglong *local_88;
  undefined8 local_80;
  int iStack_78;
  int iStack_74;
  longlong local_70;
  undefined8 local_68;
  undefined8 *local_60;
  undefined8 *puStack_58;
  longlong local_50;
  ulonglong local_48;
  ulonglong local_40;
  
  local_68 = 0xfffffffffffffffe;
  local_40 = DAT_18131ef10 ^ (ulonglong)auStack_d8;
  local_70 = param_1;
  plVar9 = (longlong *)(**(code **)(*param_2 + 0xb0))(param_2);
  plVar10 = (longlong *)(**(code **)(*param_2 + 0xb0))(param_2);
  plVar11 = (longlong *)(**(code **)(*param_2 + 0xb0))(param_2);
  plVar12 = (longlong *)(**(code **)(*param_2 + 0xa8))(param_2);
  plVar13 = (longlong *)(**(code **)(*param_2 + 0xa8))(param_2);
  lVar14 = (**(code **)(*plVar10 + 8))(plVar10,&local_60);
  uVar18 = *(undefined8 *)(lVar14 + 0x10);
  iVar5 = (**(code **)(*plVar9 + 0x20))(plVar9);
  iVar5 = (int)uVar18 + 1 + iVar5;
  uVar6 = (**(code **)(*plVar11 + 0x18))(plVar11);
  iVar7 = (**(code **)(*plVar12 + 0x20))(plVar12);
  iVar7 = iVar7 + 1;
  DAT_18133c1e8 = (**(code **)(*plVar13 + 0x18))(plVar13);
  local_80 = (longlong *)CONCAT44(uVar6,DAT_18133c1e8);
  uRam000000018133c1ec = uVar6;
  iRam000000018133c1f0 = iVar7;
  iRam000000018133c1f4 = iVar5;
  iStack_78 = iVar7;
  iStack_74 = iVar5;
  if (0xf < local_48) {
    FUN_1800409e0(&local_60,local_60,local_48 + 1);
  }
  local_48 = 0xf;
  ppppuVar19 = (ulonglong ****)0x0;
  local_50 = 0;
  local_60 = (undefined8 *)((ulonglong)local_60 & 0xffffffffffffff00);
  plVar9 = (longlong *)(**(code **)(*param_2 + 0xa8))(param_2);
  DAT_18131e000 = (**(code **)(*plVar9 + 0x18))(plVar9);
  local_80 = operator_new(0x2c8);
  ppppuVar15 = ppppuVar19;
  if (local_80 != (void *)0x0) {
    local_b0 = 0;
    local_b8 = 0;
    ppppuVar15 = (ulonglong ****)FUN_1808d8dc0(local_80,0x57,0,0);
  }
  *(undefined1 *)((longlong)ppppuVar15 + 0x183) = 1;
  local_98 = (ulonglong ***)ppppuVar15;
  local_80 = operator_new(0x2c8);
  ppppuVar16 = ppppuVar19;
  if (local_80 != (void *)0x0) {
    local_b0 = 0;
    local_b8 = 0;
    ppppuVar16 = (ulonglong ****)FUN_1808d8dc0(local_80,0x54,0);
  }
  local_90 = (ulonglong ***)ppppuVar16;
  pvVar17 = operator_new(0x2c8);
  local_80 = pvVar17;
  if (pvVar17 != (void *)0x0) {
    uVar18 = FUN_1808e3a40(1,0,1);
    local_b0 = 0;
    local_b8 = 0;
    ppppuVar19 = (ulonglong ****)FUN_1808d8dc0(pvVar17,0x58,uVar18,ppppuVar16);
  }
  lVar14 = *(longlong *)(*(longlong *)(param_1 + 0x10) + -8);
  ppppuVar2 = *(ulonglong *****)(lVar14 + 0x10);
  local_a0 = (ulonglong ***)ppppuVar19;
  if ((&local_98 < ppppuVar2) &&
     (ppppuVar21 = *(ulonglong *****)(lVar14 + 8), ppppuVar21 <= &local_98)) {
    if (ppppuVar2 == *(ulonglong *****)(lVar14 + 0x18)) {
      FUN_1800db6d0(lVar14 + 8,1);
    }
    if (*(undefined8 **)(lVar14 + 0x10) != (undefined8 *)0x0) {
      **(undefined8 **)(lVar14 + 0x10) =
           *(undefined8 *)
            (*(longlong *)(lVar14 + 8) + ((longlong)&local_98 - (longlong)ppppuVar21 >> 3) * 8);
    }
  }
  else {
    if (ppppuVar2 == *(ulonglong *****)(lVar14 + 0x18)) {
      FUN_1800db6d0(lVar14 + 8,1);
    }
    if (*(ulonglong **)(lVar14 + 0x10) != (ulonglong *)0x0) {
      **(ulonglong **)(lVar14 + 0x10) = (ulonglong)ppppuVar15;
    }
  }
  *(longlong *)(lVar14 + 0x10) = *(longlong *)(lVar14 + 0x10) + 8;
  lVar14 = *(longlong *)(param_1 + 0x100);
  uVar18 = FUN_1800ad350(param_2);
  uVar18 = (**(code **)(lVar14 + 8))(param_1 + 0x100,uVar18);
  *(undefined8 *)(param_1 + 0x188) = uVar18;
  local_80 = operator_new(0x2c8);
  if (local_80 == (longlong *)0x0) {
    ppuVar20 = (ulonglong **)0x0;
  }
  else {
    local_b0 = 0;
    local_b8 = 0;
    ppuVar20 = (ulonglong **)FUN_1808d8dc0(local_80,0x2d,*(undefined8 *)(param_1 + 0x188),0);
  }
  pppuVar3 = ppppuVar15[2];
  local_88 = (ulonglong *)ppuVar20;
  if ((&local_88 < pppuVar3) && (pppuVar4 = ppppuVar15[1], pppuVar4 <= &local_88)) {
    if (pppuVar3 == ppppuVar15[3]) {
      FUN_1800db6d0(ppppuVar15 + 1,1);
    }
    if (ppppuVar15[2] != (ulonglong ***)0x0) {
      *ppppuVar15[2] = ppppuVar15[1][(longlong)&local_88 - (longlong)pppuVar4 >> 3];
    }
  }
  else {
    if (pppuVar3 == ppppuVar15[3]) {
      FUN_1800db6d0(ppppuVar15 + 1,1);
    }
    if (ppppuVar15[2] != (ulonglong ***)0x0) {
      *ppppuVar15[2] = ppuVar20;
    }
  }
  ppppuVar15[2] = ppppuVar15[2] + 1;
  ppppuVar2 = (ulonglong ****)ppppuVar15[2];
  if ((&local_a0 < ppppuVar2) &&
     (ppppuVar21 = (ulonglong ****)ppppuVar15[1], ppppuVar21 <= &local_a0)) {
    if (ppppuVar2 == (ulonglong ****)ppppuVar15[3]) {
      FUN_1800db6d0(ppppuVar15 + 1,1);
    }
    if (ppppuVar15[2] != (ulonglong ***)0x0) {
      *ppppuVar15[2] = ppppuVar15[1][(longlong)&local_a0 - (longlong)ppppuVar21 >> 3];
    }
  }
  else {
    if (ppppuVar2 == (ulonglong ****)ppppuVar15[3]) {
      FUN_1800db6d0(ppppuVar15 + 1,1);
    }
    if (ppppuVar15[2] != (ulonglong ***)0x0) {
      *ppppuVar15[2] = (ulonglong **)ppppuVar19;
    }
  }
  ppppuVar15[2] = ppppuVar15[2] + 1;
  ppppuVar19 = *(ulonglong *****)(param_1 + 0x10);
  if ((&local_98 < ppppuVar19) &&
     (ppppuVar2 = *(ulonglong *****)(param_1 + 8), ppppuVar2 <= &local_98)) {
    if (ppppuVar19 == *(ulonglong *****)(param_1 + 0x18)) {
      FUN_1800db6d0(param_1 + 8,1);
    }
    if (*(undefined8 **)(param_1 + 0x10) != (undefined8 *)0x0) {
      **(undefined8 **)(param_1 + 0x10) =
           *(undefined8 *)
            (*(longlong *)(param_1 + 8) + ((longlong)&local_98 - (longlong)ppppuVar2 >> 3) * 8);
    }
  }
  else {
    if (ppppuVar19 == *(ulonglong *****)(param_1 + 0x18)) {
      FUN_1800db6d0(param_1 + 8,1);
    }
    if (*(ulonglong **)(param_1 + 0x10) != (ulonglong *)0x0) {
      **(ulonglong **)(param_1 + 0x10) = (ulonglong)ppppuVar15;
    }
  }
  *(longlong *)(param_1 + 0x10) = *(longlong *)(param_1 + 0x10) + 8;
  ppppuVar15 = *(ulonglong *****)(param_1 + 0x10);
  if ((&local_90 < ppppuVar15) &&
     (ppppuVar19 = *(ulonglong *****)(param_1 + 8), ppppuVar19 <= &local_90)) {
    if (ppppuVar15 == *(ulonglong *****)(param_1 + 0x18)) {
      FUN_1800db6d0(param_1 + 8,1);
    }
    if (*(undefined8 **)(param_1 + 0x10) != (undefined8 *)0x0) {
      **(undefined8 **)(param_1 + 0x10) =
           *(undefined8 *)
            (*(longlong *)(param_1 + 8) + ((longlong)&local_90 - (longlong)ppppuVar19 >> 3) * 8);
    }
  }
  else {
    if (ppppuVar15 == *(ulonglong *****)(param_1 + 0x18)) {
      FUN_1800db6d0(param_1 + 8,1);
    }
    if (*(ulonglong **)(param_1 + 0x10) != (ulonglong *)0x0) {
      **(ulonglong **)(param_1 + 0x10) = (ulonglong)ppppuVar16;
    }
  }
  *(longlong *)(param_1 + 0x10) = *(longlong *)(param_1 + 0x10) + 8;
  FUN_180093a90(param_2[1],&local_60);
  if (local_60 != puStack_58) {
    plVar9 = (longlong *)(param_1 + 0xe0);
    puVar24 = local_60;
    local_80 = plVar9;
    do {
      local_90 = (ulonglong ***)*puVar24;
      ppuVar20 = (ulonglong **)*plVar9;
      cVar1 = *(char *)((longlong)ppuVar20[1] + 0x19);
      ppuVar26 = ppuVar20;
      ppuVar25 = (ulonglong **)ppuVar20[1];
      while (cVar1 == '\0') {
        if (ppuVar25[4] < local_90) {
          ppuVar23 = (ulonglong **)ppuVar25[2];
          ppuVar25 = ppuVar26;
        }
        else {
          ppuVar23 = (ulonglong **)*ppuVar25;
        }
        ppuVar26 = ppuVar25;
        ppuVar25 = ppuVar23;
        cVar1 = *(char *)((longlong)ppuVar23 + 0x19);
      }
      if ((ppuVar26 == ppuVar20) || (local_90 < ppuVar26[4])) {
        local_a0 = (ulonglong ***)&local_90;
        local_b8 = FUN_1800d3bc0(plVar9,&DAT_180e3786a,&local_a0,local_a8);
        FUN_1800d5d10(plVar9,&local_88,ppuVar26,local_b8 + 0x20);
        ppuVar26 = (ulonglong **)local_88;
      }
      plVar10 = (longlong *)*ppuVar26[5];
      plVar11 = (longlong *)*plVar10;
      if (plVar11 != plVar10) {
        ppppuVar15 = (ulonglong ****)(param_1 + 0xf0);
        local_90 = (ulonglong ***)ppppuVar15;
        do {
          ppppuVar19 = (ulonglong ****)(plVar11 + 4);
          ppppuVar16 = (ulonglong ****)FUN_18052e9d0(ppppuVar15,ppppuVar19);
          if (ppppuVar16 == (ulonglong ****)*ppppuVar15) {
LAB_1800deaa4:
            local_a0 = (ulonglong ***)ppppuVar19;
            local_b8 = FUN_1800d3d40(ppppuVar15,&DAT_180e3786a,&local_a0,local_a8);
            FUN_1800d5160(ppppuVar15,&local_98,ppppuVar16,local_b8 + 0x20);
            ppppuVar16 = (ulonglong ****)local_98;
          }
          else {
            ppppuVar15 = ppppuVar16 + 4;
            local_a0 = ppppuVar16[6];
            if ((ulonglong ***)0xf < ppppuVar16[7]) {
              ppppuVar15 = (ulonglong ****)*ppppuVar15;
            }
            ppppuVar2 = (ulonglong ****)plVar11[6];
            ppppuVar21 = ppppuVar19;
            if (0xf < (ulonglong)plVar11[7]) {
              ppppuVar21 = (ulonglong ****)*ppppuVar19;
            }
            _Size = (ulonglong ****)local_a0;
            if (ppppuVar2 < local_a0) {
              _Size = ppppuVar2;
            }
            if (_Size == (ulonglong ****)0x0) {
              uVar8 = 0;
            }
            else {
              uVar8 = memcmp(ppppuVar21,ppppuVar15,(size_t)_Size);
            }
            if (uVar8 == 0) {
              if (ppppuVar2 < local_a0) {
                uVar8 = 0xffffffff;
              }
              else {
                uVar8 = (uint)(ppppuVar2 != (ulonglong ****)local_a0);
              }
            }
            ppppuVar15 = (ulonglong ****)local_90;
            if ((int)uVar8 < 0) goto LAB_1800deaa4;
          }
          ppppuVar16[8] = (ulonglong ***)plVar11[8];
          if (*(char *)((longlong)plVar11 + 0x19) == '\0') {
            plVar9 = (longlong *)plVar11[2];
            if (*(char *)((longlong)plVar9 + 0x19) == '\0') {
              cVar1 = *(char *)(*plVar9 + 0x19);
              plVar11 = plVar9;
              plVar9 = (longlong *)*plVar9;
              while (cVar1 == '\0') {
                cVar1 = *(char *)(*plVar9 + 0x19);
                plVar11 = plVar9;
                plVar9 = (longlong *)*plVar9;
              }
            }
            else {
              cVar1 = *(char *)(plVar11[1] + 0x19);
              plVar12 = (longlong *)plVar11[1];
              plVar9 = plVar11;
              while ((plVar11 = plVar12, cVar1 == '\0' && (plVar9 == (longlong *)plVar11[2]))) {
                cVar1 = *(char *)(plVar11[1] + 0x19);
                plVar12 = (longlong *)plVar11[1];
                plVar9 = plVar11;
              }
            }
          }
          plVar9 = local_80;
          param_1 = local_70;
        } while (plVar11 != plVar10);
      }
      puVar24 = puVar24 + 1;
    } while (puVar24 != puStack_58);
  }
  if (local_60 != (undefined8 *)0x0) {
    uVar22 = local_50 - (longlong)local_60 >> 3;
    if (0x1fffffffffffffff < uVar22) {
                    /* WARNING: Subroutine does not return */
      _invalid_parameter_noinfo_noreturn();
    }
    puVar24 = local_60;
    if (0xfff < uVar22 * 8) {
      if (((ulonglong)local_60 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
        _invalid_parameter_noinfo_noreturn();
      }
      puVar24 = (undefined8 *)local_60[-1];
      if (local_60 <= puVar24) {
                    /* WARNING: Subroutine does not return */
        _invalid_parameter_noinfo_noreturn();
      }
      if ((ulonglong)((longlong)local_60 - (longlong)puVar24) < 8) {
                    /* WARNING: Subroutine does not return */
        _invalid_parameter_noinfo_noreturn();
      }
      if (0x27 < (ulonglong)((longlong)local_60 - (longlong)puVar24)) {
                    /* WARNING: Subroutine does not return */
        _invalid_parameter_noinfo_noreturn();
      }
    }
    free(puVar24);
  }
  return;
}



===== 0x1800f02f0 1800f02f0 FUN_1800f02f0 =====

void FUN_1800f02f0(longlong param_1)

{
  *(undefined1 *)(param_1 + 0x183) = 0;
  *(longlong *)(param_1 + 0x98) = *(longlong *)(param_1 + 0x98) + -1;
  *(longlong *)(param_1 + 0x10) = *(longlong *)(param_1 + 0x10) + -8;
  return;
}



===== 0x1801c7b80 1801c7b80 FUN_1801c7b80 =====

void FUN_1801c7b80(longlong param_1,undefined8 param_2)

{
  longlong *plVar1;
  undefined8 *puVar2;
  int iVar3;
  undefined8 *puVar4;
  void **ppvVar5;
  void **ppvVar6;
  longlong lVar7;
  longlong lVar8;
  undefined8 uVar9;
  void *pvVar10;
  void *pvVar11;
  void *pvVar12;
  void *pvVar13;
  void *pvVar14;
  longlong *plVar15;
  ulonglong uVar16;
  undefined8 *puVar17;
  char cVar18;
  ulonglong *puVar19;
  longlong *plVar20;
  ulonglong uVar21;
  longlong *plVar22;
  void *local_res18;
  void *local_res20;
  void *local_88;
  void *local_80;
  undefined8 local_78;
  void *local_70;
  void *local_68;
  undefined8 uStack_60;
  longlong local_58;
  
  local_78 = 0xfffffffffffffffe;
  lVar8 = FUN_180165040(param_2);
  pvVar14 = (void *)0x0;
  if (lVar8 == 0) goto LAB_1801c826b;
  lVar8 = DAT_18133caf0[-1];
  if (*(int *)(lVar8 + 4) != 0xaa) {
    DAT_18133caf0 = DAT_18133caf0 + -1;
    return;
  }
  puVar2 = (undefined8 *)(lVar8 + 0x188);
  uVar9 = FUN_1808db990(lVar8);
  *puVar2 = uVar9;
  lVar7 = DAT_18137e938;
  puVar19 = (ulonglong *)(DAT_18137e938 + 0x170);
  puVar17 = *(undefined8 **)(DAT_18137e938 + 0x178);
  if ((puVar2 < puVar17) && (puVar4 = (undefined8 *)*puVar19, puVar4 <= puVar2)) {
    if (puVar17 == *(undefined8 **)(DAT_18137e938 + 0x180)) {
      FUN_1800db6d0(puVar19);
    }
    puVar17 = *(undefined8 **)(lVar7 + 0x178);
    if (puVar17 != (undefined8 *)0x0) {
      uVar9 = *(undefined8 *)(*puVar19 + ((longlong)puVar2 - (longlong)puVar4 >> 3) * 8);
LAB_1801c7c6a:
      *puVar17 = uVar9;
    }
  }
  else {
    if (puVar17 == *(undefined8 **)(DAT_18137e938 + 0x180)) {
      FUN_1800db6d0(puVar19);
    }
    puVar17 = *(undefined8 **)(lVar7 + 0x178);
    if (puVar17 != (undefined8 *)0x0) {
      uVar9 = *puVar2;
      goto LAB_1801c7c6a;
    }
  }
  *(longlong *)(lVar7 + 0x178) = *(longlong *)(lVar7 + 0x178) + 8;
  *(undefined1 *)(lVar8 + 0x183) = 1;
  *(undefined4 *)(lVar8 + 4) = 0x57;
  local_res20 = operator_new(0x2c8);
  pvVar10 = pvVar14;
  if (local_res20 != (void *)0x0) {
    pvVar10 = (void *)FUN_1808d8dc0(local_res20,0x54,0,0,0,0);
  }
  local_res18 = pvVar10;
  pvVar11 = operator_new(0x2c8);
  pvVar12 = pvVar14;
  local_res20 = pvVar11;
  if (pvVar11 != (void *)0x0) {
    uVar9 = FUN_1808e3a40(1,0,1);
    pvVar12 = (void *)FUN_1808d8dc0(pvVar11,0x58,uVar9,pvVar10,0,0);
  }
  local_80 = pvVar12;
  local_res20 = operator_new(0x2c8);
  pvVar11 = pvVar14;
  if (local_res20 != (void *)0x0) {
    pvVar11 = (void *)FUN_1808d8dc0(local_res20,0x54,0,0,0,0);
  }
  local_res20 = pvVar11;
  pvVar13 = operator_new(0x2c8);
  if (pvVar13 != (void *)0x0) {
    local_88 = pvVar13;
    local_70 = operator_new(0x2c8);
    if (local_70 != (void *)0x0) {
      pvVar14 = (void *)FUN_1808d8dc0(local_70,0x5b,0,0,0,0);
    }
    pvVar14 = (void *)FUN_1808d8dc0(pvVar13,0x58,pvVar14,0,0,0);
  }
  pvVar13 = local_res18;
  plVar15 = (longlong *)(lVar8 + 8);
  local_88 = pvVar14;
  if ((*(longlong *)(lVar8 + 0x10) - *plVar15 >> 3 != 0) &&
     (*(int *)(*(longlong *)*plVar15 + 4) == 0x58)) {
    *(undefined4 *)(*(longlong *)*plVar15 + 4) = 0x2d;
    plVar20 = *(longlong **)(*(longlong *)*plVar15 + 8);
    if (plVar20 != *(longlong **)(*(longlong *)*plVar15 + 0x10)) {
      plVar22 = plVar20 + 1;
      do {
        if ((*(int *)(*plVar20 + 4) == 0x54) || (*(int *)(*plVar20 + 4) == 99)) {
          FUN_180132830((longlong)pvVar13 + 8,plVar20);
          lVar7 = *(longlong *)*plVar15;
          memmove(plVar20,plVar22,(*(longlong *)(lVar7 + 0x10) - (longlong)plVar22 >> 3) * 8);
          plVar1 = (longlong *)(lVar7 + 0x10);
          *plVar1 = *plVar1 + -8;
        }
        else {
          plVar20 = plVar20 + 1;
          plVar22 = plVar22 + 1;
        }
        pvVar10 = local_res18;
      } while (plVar20 != *(longlong **)(*(longlong *)*plVar15 + 0x10));
    }
    *(undefined4 *)((longlong)pvVar10 + 0x200) = *(undefined4 *)(*(longlong *)*plVar15 + 0x214);
    *(undefined4 *)((longlong)pvVar10 + 0x214) = *(undefined4 *)(*(longlong *)*plVar15 + 0x214);
    *(undefined4 *)((longlong)pvVar12 + 0x214) = *(undefined4 *)(*(longlong *)*plVar15 + 0x214);
    *(undefined4 *)((longlong)pvVar12 + 0x200) = *(undefined4 *)(*(longlong *)*plVar15 + 0x214);
    *(undefined4 *)(**(longlong **)((longlong)pvVar12 + 8) + 0x214) =
         *(undefined4 *)(*(longlong *)*plVar15 + 0x214);
    *(undefined4 *)(**(longlong **)((longlong)pvVar12 + 8) + 0x200) =
         *(undefined4 *)(*(longlong *)*plVar15 + 0x214);
    FUN_1801a6990(plVar15,&local_res18,*plVar15 + 8,1,&local_80);
  }
  cVar18 = '\0';
  local_res18 = (void *)((ulonglong)local_res18 & 0xffffffffffffff00);
  pvVar10 = (void *)0x0;
  plVar20 = (longlong *)*plVar15;
  if (plVar20 != *(longlong **)(lVar8 + 0x10)) {
    plVar22 = plVar20 + 1;
    do {
      pvVar12 = (void *)*plVar20;
      if (*(int *)((longlong)pvVar12 + 4) == 0xab) {
        local_res18 = (void *)CONCAT71(local_res18._1_7_,1);
        *(undefined4 *)((longlong)pvVar14 + 0x214) = *(undefined4 *)((longlong)pvVar12 + 0x214);
        *(undefined4 *)((longlong)pvVar14 + 0x200) = *(undefined4 *)(*plVar20 + 0x214);
        *(undefined4 *)(**(longlong **)((longlong)pvVar14 + 8) + 0x214) =
             *(undefined4 *)(*plVar20 + 0x214);
        *(undefined4 *)(**(longlong **)((longlong)pvVar14 + 8) + 0x200) =
             *(undefined4 *)(*plVar20 + 0x214);
        *(undefined4 *)((longlong)pvVar11 + 0x214) = *(undefined4 *)(*plVar20 + 0x214);
        *(undefined4 *)((longlong)pvVar11 + 0x200) = *(undefined4 *)(*plVar20 + 0x214);
        FUN_180132830((longlong)pvVar11 + 8,plVar20);
        memmove(plVar20,plVar22,(*(longlong *)(lVar8 + 0x10) - (longlong)plVar22 >> 3) * 8);
        *(longlong *)(lVar8 + 0x10) = *(longlong *)(lVar8 + 0x10) + -8;
        cVar18 = '\x01';
      }
      else if (*(int *)((longlong)pvVar12 + 4) == 0xac) {
        if (cVar18 == '\x01') {
          *(undefined4 *)((longlong)pvVar11 + 0x214) = *(undefined4 *)((longlong)pvVar12 + 0x214);
          *(undefined4 *)((longlong)pvVar11 + 0x200) = *(undefined4 *)(*plVar20 + 0x214);
          FUN_180132830((longlong)pvVar11 + 8,plVar20);
        }
        memmove(plVar20,plVar22,(*(longlong *)(lVar8 + 0x10) - (longlong)plVar22 >> 3) * 8);
        *(longlong *)(lVar8 + 0x10) = *(longlong *)(lVar8 + 0x10) + -8;
        pvVar10 = pvVar12;
        cVar18 = (char)local_res18;
      }
      else {
        plVar20 = plVar20 + 1;
        plVar22 = plVar22 + 1;
      }
    } while (plVar20 != *(longlong **)(lVar8 + 0x10));
    if (cVar18 != '\0') {
      FUN_180132830((longlong)pvVar14 + 8,&local_res20);
      FUN_1801a6990(plVar15,&local_70,*plVar15 + 0x10,1,&local_88);
      pvVar11 = local_res20;
    }
  }
  FUN_1801ad560(pvVar11);
  if ((char)local_res18 == '\0') {
    if (pvVar10 == (void *)0x0) {
      if (pvVar14 != (void *)0x0) {
        FUN_1808d9570(pvVar14);
        free(pvVar14);
      }
      if (pvVar11 == (void *)0x0) goto LAB_1801c825c;
      FUN_1808d9570(pvVar11);
    }
    else {
      *(undefined4 *)((longlong)pvVar14 + 0x214) = *(undefined4 *)((longlong)pvVar10 + 0x214);
      *(undefined4 *)((longlong)pvVar14 + 0x200) = *(undefined4 *)((longlong)pvVar10 + 0x214);
      *(undefined4 *)(**(longlong **)((longlong)pvVar14 + 8) + 0x214) =
           *(undefined4 *)((longlong)pvVar10 + 0x214);
      *(undefined4 *)(**(longlong **)((longlong)pvVar14 + 8) + 0x200) =
           *(undefined4 *)((longlong)pvVar10 + 0x214);
      *(undefined4 *)((longlong)pvVar11 + 0x214) = *(undefined4 *)((longlong)pvVar10 + 0x214);
      *(undefined4 *)((longlong)pvVar11 + 0x200) = *(undefined4 *)((longlong)pvVar10 + 0x214);
      ppvVar5 = *(void ***)((longlong)pvVar14 + 0x10);
      if ((&local_res20 < ppvVar5) &&
         (ppvVar6 = *(void ***)((longlong)pvVar14 + 8), ppvVar6 <= &local_res20)) {
        if (ppvVar5 == *(void ***)((longlong)pvVar14 + 0x18)) {
          FUN_1800db6d0((longlong)pvVar14 + 8,1);
        }
        if (*(undefined8 **)((longlong)pvVar14 + 0x10) != (undefined8 *)0x0) {
          **(undefined8 **)((longlong)pvVar14 + 0x10) =
               *(undefined8 *)
                (*(longlong *)((longlong)pvVar14 + 8) +
                ((longlong)&local_res20 - (longlong)ppvVar6 >> 3) * 8);
        }
      }
      else {
        if (ppvVar5 == *(void ***)((longlong)pvVar14 + 0x18)) {
          FUN_1800db6d0((longlong)pvVar14 + 8,1);
        }
        if (*(undefined8 **)((longlong)pvVar14 + 0x10) != (undefined8 *)0x0) {
          **(undefined8 **)((longlong)pvVar14 + 0x10) = pvVar11;
        }
      }
      *(longlong *)((longlong)pvVar14 + 0x10) = *(longlong *)((longlong)pvVar14 + 0x10) + 8;
      FUN_1801a6990(plVar15,&local_res18,*plVar15 + 0x10,1,&local_88);
      plVar15 = *(longlong **)((longlong)pvVar10 + 8);
      if (plVar15 != *(longlong **)((longlong)pvVar10 + 0x10)) {
        plVar20 = plVar15 + 1;
        do {
          iVar3 = *(int *)(*plVar15 + 4);
          if ((iVar3 == 0x54) || (iVar3 == 99)) {
            *(undefined4 *)((longlong)pvVar11 + 0x214) = *(undefined4 *)(*plVar15 + 0x214);
            *(undefined4 *)((longlong)pvVar11 + 0x200) = *(undefined4 *)(*plVar15 + 0x214);
            FUN_180132830((longlong)pvVar11 + 8,plVar15);
            memmove(plVar15,plVar20,
                    (*(longlong *)((longlong)pvVar10 + 0x10) - (longlong)plVar20 >> 3) * 8);
            *(longlong *)((longlong)pvVar10 + 0x10) = *(longlong *)((longlong)pvVar10 + 0x10) + -8;
          }
          else {
            plVar15 = plVar15 + 1;
            plVar20 = plVar20 + 1;
          }
        } while (plVar15 != *(longlong **)((longlong)pvVar10 + 0x10));
      }
      FUN_1808d9570(pvVar10);
      pvVar11 = pvVar10;
    }
    free(pvVar11);
  }
LAB_1801c825c:
  DAT_18133caf0 = DAT_18133caf0 + -1;
LAB_1801c826b:
  uVar21 = 0;
  do {
    plVar15 = (longlong *)FUN_180164f80(param_2,&local_68);
    lVar8 = plVar15[1];
    lVar7 = *plVar15;
    if (local_68 != (void *)0x0) {
      uVar16 = local_58 - (longlong)local_68 >> 3;
      if (0x1fffffffffffffff < uVar16) {
                    /* WARNING: Subroutine does not return */
        _invalid_parameter_noinfo_noreturn();
      }
      pvVar14 = local_68;
      if (0xfff < uVar16 * 8) {
        if (((ulonglong)local_68 & 0x1f) != 0) {
                    /* WARNING: Subroutine does not return */
          _invalid_parameter_noinfo_noreturn();
        }
        pvVar14 = *(void **)((longlong)local_68 + -8);
        if (local_68 <= pvVar14) {
                    /* WARNING: Subroutine does not return */
          _invalid_parameter_noinfo_noreturn();
        }
        if ((ulonglong)((longlong)local_68 - (longlong)pvVar14) < 8) {
                    /* WARNING: Subroutine does not return */
          _invalid_parameter_noinfo_noreturn();
        }
        if (0x27 < (ulonglong)((longlong)local_68 - (longlong)pvVar14)) {
                    /* WARNING: Subroutine does not return */
          _invalid_parameter_noinfo_noreturn();
        }
      }
      free(pvVar14);
      local_68 = (void *)0x0;
      uStack_60 = 0;
      local_58 = 0;
    }
    if ((ulonglong)(lVar8 - lVar7 >> 3) <= uVar21) {
      lVar8 = FUN_180164f60(param_2);
      if (lVar8 != 0) {
        local_res18 = (void *)FUN_180164f60(param_2);
        lVar8 = FUN_180846bc0(param_1 + 0x10,&local_res18);
        if (lVar8 != 0) {
          local_res18 = (void *)FUN_180164f60(param_2);
          FUN_1800e9650(param_1 + 0x10,&local_res18);
        }
      }
      return;
    }
    local_res18 = (void *)FUN_180164fc0(param_2,uVar21);
    lVar8 = FUN_180846bc0(param_1 + 0x10,&local_res18);
    if (lVar8 != 0) {
      local_res18 = (void *)FUN_180164fc0(param_2,uVar21);
      FUN_1800e9650(param_1 + 0x10,&local_res18);
    }
    uVar21 = uVar21 + 1;
  } while( true );
}



===== 0x1808d8dc0 1808d8dc0 FUN_1808d8dc0 =====

uint * FUN_1808d8dc0(uint *param_1,uint param_2,longlong param_3,longlong param_4,longlong param_5,
                    longlong param_6)

{
  uint *puVar1;
  uint uVar2;
  uint uVar3;
  undefined8 uVar4;
  uint uVar5;
  undefined4 local_res10 [2];
  longlong local_res18;
  longlong local_res20;
  undefined8 uVar6;
  uint *puVar7;
  
  uVar6 = 0xfffffffffffffffe;
  param_1[2] = 0;
  param_1[3] = 0;
  param_1[4] = 0;
  param_1[5] = 0;
  param_1[6] = 0;
  param_1[7] = 0;
  puVar7 = param_1 + 8;
  puVar7[0] = 0;
  puVar7[1] = 0;
  param_1[10] = 0;
  param_1[0xb] = 0;
  local_res18 = param_3;
  local_res20 = param_4;
  uVar4 = FUN_1801a68a0(puVar7);
  *(undefined8 *)puVar7 = uVar4;
  param_1[0xc] = 0;
  param_1[0xd] = 0;
  param_1[0xe] = 0;
  param_1[0xf] = 0;
  param_1[0x10] = 0;
  param_1[0x11] = 0;
  param_1[0x18] = 0xf;
  param_1[0x19] = 0;
  param_1[0x16] = 0;
  param_1[0x17] = 0;
  *(undefined1 *)(param_1 + 0x12) = 0;
  param_1[0x20] = 0xf;
  param_1[0x21] = 0;
  param_1[0x1e] = 0;
  param_1[0x1f] = 0;
  *(undefined1 *)(param_1 + 0x1a) = 0;
  param_1[0x22] = 0;
  param_1[0x23] = 0;
  param_1[0x24] = 0;
  param_1[0x25] = 0;
  param_1[0x26] = 0;
  param_1[0x27] = 0;
  param_1[0x3a] = 0xf;
  param_1[0x3b] = 0;
  param_1[0x38] = 0;
  param_1[0x39] = 0;
  *(undefined1 *)(param_1 + 0x34) = 0;
  param_1[0x4c] = 0xf;
  param_1[0x4d] = 0;
  param_1[0x4a] = 0;
  param_1[0x4b] = 0;
  *(undefined1 *)(param_1 + 0x46) = 0;
  param_1[0x54] = 0xf;
  param_1[0x55] = 0;
  param_1[0x52] = 0;
  param_1[0x53] = 0;
  *(undefined1 *)(param_1 + 0x4e) = 0;
  param_1[0x5c] = 0xf;
  param_1[0x5d] = 0;
  param_1[0x5a] = 0;
  param_1[0x5b] = 0;
  *(undefined1 *)(param_1 + 0x56) = 0;
  param_1[0x66] = 0;
  param_1[0x67] = 0;
  param_1[0x68] = 0;
  param_1[0x69] = 0;
  param_1[0x6a] = 0;
  param_1[0x6b] = 0;
  puVar7 = param_1 + 0x6c;
  local_res10[0] = 0;
  puVar7[0] = 0;
  puVar7[1] = 0;
  param_1[0x6e] = 0;
  param_1[0x6f] = 0;
  param_1[0x70] = 0;
  param_1[0x71] = 0;
  FUN_1801ec450(puVar7,0,local_res10);
  param_1[0x72] = 0;
  param_1[0x73] = 0;
  puVar1 = param_1 + 0x78;
  param_1[0x7e] = 0xf;
  param_1[0x7f] = 0;
  param_1[0x7c] = 0;
  param_1[0x7d] = 0;
  *(undefined1 *)puVar1 = 0;
  param_1[0x80] = 0;
  param_1[0x81] = 0;
  param_1[0x82] = 0;
  param_1[0x83] = 0;
  param_1[0x8c] = 0xf;
  param_1[0x8d] = 0;
  param_1[0x8a] = 0;
  param_1[0x8b] = 0;
  *(undefined1 *)(param_1 + 0x86) = 0;
  param_1[0x94] = 0xf;
  param_1[0x95] = 0;
  param_1[0x92] = 0;
  param_1[0x93] = 0;
  *(undefined1 *)(param_1 + 0x8e) = 0;
  param_1[0x9c] = 0xf;
  param_1[0x9d] = 0;
  param_1[0x9a] = 0;
  param_1[0x9b] = 0;
  *(undefined1 *)(param_1 + 0x96) = 0;
  param_1[0xa4] = 0xf;
  param_1[0xa5] = 0;
  param_1[0xa2] = 0;
  param_1[0xa3] = 0;
  *(undefined1 *)(param_1 + 0x9e) = 0;
  param_1[0xa6] = 0;
  param_1[0xa7] = 0;
  param_1[0xa8] = 0;
  param_1[0xa9] = 0;
  *(undefined2 *)(param_1 + 0xaa) = 0;
  param_1[0xac] = 0;
  param_1[0xad] = 0;
  param_1[0xae] = 0;
  param_1[0xaf] = 0;
  param_1[0xb0] = 0;
  param_1[0xb1] = 0;
  uVar5 = DAT_18131e9dc << 0xd ^ DAT_18131e9dc;
  uVar5 = uVar5 ^ uVar5 >> 0x11;
  DAT_18131e9dc = uVar5 << 5 ^ uVar5;
  *param_1 = DAT_18131e9dc;
  uVar5 = (*DAT_18137cbb0)();
  param_1[0x84] = uVar5;
  uVar3 = uRam000000018133c1f4;
  uVar2 = uRam000000018133c1f0;
  uVar5 = uRam000000018133c1ec;
  param_1[0x80] = DAT_18133c1e8;
  param_1[0x81] = uVar5;
  param_1[0x82] = uVar2;
  param_1[0x83] = uVar3;
  param_1[0x85] = 0;
  param_1[1] = param_2;
  if (puVar1 != (uint *)&DAT_18137ccc8) {
    FUN_180040550(puVar1,&DAT_18137ccc8,0,0xffffffffffffffff,uVar6,puVar7);
  }
  param_1[0x28] = 0;
  param_1[0x29] = 0;
  *(undefined1 *)(param_1 + 100) = 0;
  *(undefined2 *)(param_1 + 0x2a) = 0;
  *(undefined4 *)((longlong)param_1 + 0xab) = 0;
  *(undefined1 *)((longlong)param_1 + 0xaa) = 0;
  param_1[0x2c] = 0;
  param_1[0x2d] = 0xffffffff;
  param_1[0x2e] = 0;
  param_1[0x2f] = 0;
  param_1[0x32] = 0;
  param_1[0x33] = 0;
  param_1[0x74] = 0;
  param_1[0x75] = 0;
  *(undefined2 *)(param_1 + 0x76) = 0;
  param_1[0x40] = 0;
  param_1[0x41] = 0;
  param_1[0x42] = 0;
  param_1[0x43] = 0;
  *(undefined2 *)(param_1 + 0x5e) = 0;
  *(undefined8 *)((longlong)param_1 + 0x17b) = 0;
  *(undefined1 *)((longlong)param_1 + 0x17a) = 0;
  *(undefined1 *)(param_1 + 0x44) = 0;
  *(undefined2 *)((longlong)param_1 + 0x183) = 0;
  *(undefined1 *)((longlong)param_1 + 0x185) = 0;
  param_1[0x3c] = 0;
  param_1[0x3d] = 0;
  param_1[0x3e] = 0;
  param_1[0x3f] = 0;
  param_1[0x62] = 0;
  param_1[99] = 0;
  if (param_3 != 0) {
    FUN_180132830(param_1 + 2,&local_res18);
  }
  if (param_4 != 0) {
    FUN_180132830(param_1 + 2,&local_res20);
  }
  if (param_5 != 0) {
    FUN_180132830(param_1 + 2,&param_5);
  }
  if (param_6 != 0) {
    FUN_180132830(param_1 + 2,&param_6);
  }
  return param_1;
}



===== 0x1808d9a90 1808d9a90 FUN_1808d9a90 =====

undefined4 * FUN_1808d9a90(undefined4 *param_1,undefined4 *param_2)

{
  longlong *plVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined1 local_res8;
  
  *param_1 = *param_2;
  param_1[1] = param_2[1];
  FUN_1801ad090(param_1 + 2,param_2 + 2);
  plVar1 = (longlong *)(param_1 + 8);
  if (plVar1 != (longlong *)(param_2 + 8)) {
    FUN_1800da990(plVar1,*(undefined8 *)(*plVar1 + 8));
    *(longlong *)(*plVar1 + 8) = *plVar1;
    *(longlong *)*plVar1 = *plVar1;
    *(longlong *)(*plVar1 + 0x10) = *plVar1;
    *(undefined8 *)(param_1 + 10) = 0;
    FUN_1801a4d10(plVar1,param_2 + 8,local_res8);
  }
  FUN_1801ad090(param_1 + 0xc,param_2 + 0xc);
  if (param_1 + 0x12 != param_2 + 0x12) {
    FUN_180040550(param_1 + 0x12,param_2 + 0x12,0,0xffffffffffffffff);
  }
  if (param_1 + 0x1a != param_2 + 0x1a) {
    FUN_180040550(param_1 + 0x1a,param_2 + 0x1a,0,0xffffffffffffffff);
  }
  FUN_1801a6350(param_1 + 0x22,param_2 + 0x22);
  *(undefined1 *)(param_1 + 0x28) = *(undefined1 *)(param_2 + 0x28);
  *(undefined1 *)((longlong)param_1 + 0xa1) = *(undefined1 *)((longlong)param_2 + 0xa1);
  *(undefined1 *)((longlong)param_1 + 0xa2) = *(undefined1 *)((longlong)param_2 + 0xa2);
  *(undefined1 *)((longlong)param_1 + 0xa3) = *(undefined1 *)((longlong)param_2 + 0xa3);
  *(undefined1 *)(param_1 + 0x29) = *(undefined1 *)(param_2 + 0x29);
  *(undefined1 *)((longlong)param_1 + 0xa5) = *(undefined1 *)((longlong)param_2 + 0xa5);
  *(undefined1 *)((longlong)param_1 + 0xa6) = *(undefined1 *)((longlong)param_2 + 0xa6);
  *(undefined1 *)((longlong)param_1 + 0xa7) = *(undefined1 *)((longlong)param_2 + 0xa7);
  *(undefined1 *)(param_1 + 0x2a) = *(undefined1 *)(param_2 + 0x2a);
  *(undefined1 *)((longlong)param_1 + 0xa9) = *(undefined1 *)((longlong)param_2 + 0xa9);
  *(undefined1 *)((longlong)param_1 + 0xaa) = *(undefined1 *)((longlong)param_2 + 0xaa);
  *(undefined1 *)((longlong)param_1 + 0xab) = *(undefined1 *)((longlong)param_2 + 0xab);
  *(undefined1 *)(param_1 + 0x2b) = *(undefined1 *)(param_2 + 0x2b);
  *(undefined1 *)((longlong)param_1 + 0xad) = *(undefined1 *)((longlong)param_2 + 0xad);
  *(undefined1 *)((longlong)param_1 + 0xae) = *(undefined1 *)((longlong)param_2 + 0xae);
  param_1[0x2c] = param_2[0x2c];
  param_1[0x2d] = param_2[0x2d];
  param_1[0x2e] = param_2[0x2e];
  param_1[0x2f] = param_2[0x2f];
  param_1[0x30] = param_2[0x30];
  *(undefined8 *)(param_1 + 0x32) = *(undefined8 *)(param_2 + 0x32);
  if (param_1 + 0x34 != param_2 + 0x34) {
    FUN_180040550(param_1 + 0x34,param_2 + 0x34,0,0xffffffffffffffff);
  }
  uVar2 = param_2[0x3d];
  uVar3 = param_2[0x3e];
  uVar4 = param_2[0x3f];
  param_1[0x3c] = param_2[0x3c];
  param_1[0x3d] = uVar2;
  param_1[0x3e] = uVar3;
  param_1[0x3f] = uVar4;
  *(undefined1 *)(param_1 + 0x40) = *(undefined1 *)(param_2 + 0x40);
  *(undefined1 *)((longlong)param_1 + 0x101) = *(undefined1 *)((longlong)param_2 + 0x101);
  *(undefined1 *)((longlong)param_1 + 0x102) = *(undefined1 *)((longlong)param_2 + 0x102);
  *(undefined1 *)((longlong)param_1 + 0x103) = *(undefined1 *)((longlong)param_2 + 0x103);
  *(undefined1 *)(param_1 + 0x41) = *(undefined1 *)(param_2 + 0x41);
  *(undefined1 *)((longlong)param_1 + 0x105) = *(undefined1 *)((longlong)param_2 + 0x105);
  *(undefined1 *)((longlong)param_1 + 0x106) = *(undefined1 *)((longlong)param_2 + 0x106);
  *(undefined1 *)((longlong)param_1 + 0x107) = *(undefined1 *)((longlong)param_2 + 0x107);
  *(undefined1 *)(param_1 + 0x42) = *(undefined1 *)(param_2 + 0x42);
  *(undefined1 *)((longlong)param_1 + 0x109) = *(undefined1 *)((longlong)param_2 + 0x109);
  *(undefined1 *)((longlong)param_1 + 0x10a) = *(undefined1 *)((longlong)param_2 + 0x10a);
  *(undefined1 *)((longlong)param_1 + 0x10b) = *(undefined1 *)((longlong)param_2 + 0x10b);
  *(undefined1 *)(param_1 + 0x43) = *(undefined1 *)(param_2 + 0x43);
  *(undefined1 *)((longlong)param_1 + 0x10d) = *(undefined1 *)((longlong)param_2 + 0x10d);
  *(undefined1 *)((longlong)param_1 + 0x10e) = *(undefined1 *)((longlong)param_2 + 0x10e);
  *(undefined1 *)((longlong)param_1 + 0x10f) = *(undefined1 *)((longlong)param_2 + 0x10f);
  *(undefined1 *)(param_1 + 0x44) = *(undefined1 *)(param_2 + 0x44);
  if (param_1 + 0x46 != param_2 + 0x46) {
    FUN_180040550(param_1 + 0x46,param_2 + 0x46,0,0xffffffffffffffff);
  }
  if (param_1 + 0x4e != param_2 + 0x4e) {
    FUN_180040550(param_1 + 0x4e,param_2 + 0x4e,0,0xffffffffffffffff);
  }
  if (param_1 + 0x56 != param_2 + 0x56) {
    FUN_180040550(param_1 + 0x56,param_2 + 0x56,0,0xffffffffffffffff);
  }
  *(undefined1 *)(param_1 + 0x5e) = *(undefined1 *)(param_2 + 0x5e);
  *(undefined1 *)((longlong)param_1 + 0x179) = *(undefined1 *)((longlong)param_2 + 0x179);
  *(undefined1 *)((longlong)param_1 + 0x17a) = *(undefined1 *)((longlong)param_2 + 0x17a);
  *(undefined1 *)((longlong)param_1 + 0x17b) = *(undefined1 *)((longlong)param_2 + 0x17b);
  *(undefined1 *)(param_1 + 0x5f) = *(undefined1 *)(param_2 + 0x5f);
  *(undefined1 *)((longlong)param_1 + 0x17d) = *(undefined1 *)((longlong)param_2 + 0x17d);
  *(undefined1 *)((longlong)param_1 + 0x17e) = *(undefined1 *)((longlong)param_2 + 0x17e);
  *(undefined1 *)((longlong)param_1 + 0x17f) = *(undefined1 *)((longlong)param_2 + 0x17f);
  *(undefined1 *)(param_1 + 0x60) = *(undefined1 *)(param_2 + 0x60);
  *(undefined1 *)((longlong)param_1 + 0x181) = *(undefined1 *)((longlong)param_2 + 0x181);
  *(undefined1 *)((longlong)param_1 + 0x182) = *(undefined1 *)((longlong)param_2 + 0x182);
  *(undefined1 *)((longlong)param_1 + 0x183) = *(undefined1 *)((longlong)param_2 + 0x183);
  *(undefined1 *)(param_1 + 0x61) = *(undefined1 *)(param_2 + 0x61);
  *(undefined1 *)((longlong)param_1 + 0x185) = *(undefined1 *)((longlong)param_2 + 0x185);
  *(undefined8 *)(param_1 + 0x62) = *(undefined8 *)(param_2 + 0x62);
  *(undefined1 *)(param_1 + 100) = *(undefined1 *)(param_2 + 100);
  FUN_18020ee60(param_1 + 0x66,param_2 + 0x66);
  *(undefined8 *)(param_1 + 0x72) = *(undefined8 *)(param_2 + 0x72);
  FUN_18051c0b0();
  *(undefined8 *)(param_1 + 0x74) = *(undefined8 *)(param_2 + 0x74);
  *(undefined1 *)(param_1 + 0x76) = *(undefined1 *)(param_2 + 0x76);
  *(undefined1 *)((longlong)param_1 + 0x1d9) = *(undefined1 *)((longlong)param_2 + 0x1d9);
  if (param_1 + 0x78 != param_2 + 0x78) {
    FUN_180040550(param_1 + 0x78,param_2 + 0x78,0,0xffffffffffffffff);
  }
  uVar2 = param_2[0x81];
  uVar3 = param_2[0x82];
  uVar4 = param_2[0x83];
  param_1[0x80] = param_2[0x80];
  param_1[0x81] = uVar2;
  param_1[0x82] = uVar3;
  param_1[0x83] = uVar4;
  param_1[0x84] = param_2[0x84];
  param_1[0x85] = param_2[0x85];
  if (param_1 + 0x86 != param_2 + 0x86) {
    FUN_180040550(param_1 + 0x86,param_2 + 0x86,0,0xffffffffffffffff);
  }
  if (param_1 + 0x8e != param_2 + 0x8e) {
    FUN_180040550(param_1 + 0x8e,param_2 + 0x8e,0,0xffffffffffffffff);
  }
  if (param_1 + 0x96 != param_2 + 0x96) {
    FUN_180040550(param_1 + 0x96,param_2 + 0x96,0,0xffffffffffffffff);
  }
  if (param_1 + 0x9e != param_2 + 0x9e) {
    FUN_180040550(param_1 + 0x9e,param_2 + 0x9e,0,0xffffffffffffffff);
  }
  *(undefined1 *)(param_1 + 0xa6) = *(undefined1 *)(param_2 + 0xa6);
  *(undefined1 *)((longlong)param_1 + 0x299) = *(undefined1 *)((longlong)param_2 + 0x299);
  *(undefined1 *)((longlong)param_1 + 0x29a) = *(undefined1 *)((longlong)param_2 + 0x29a);
  *(undefined1 *)((longlong)param_1 + 0x29b) = *(undefined1 *)((longlong)param_2 + 0x29b);
  *(undefined1 *)(param_1 + 0xa7) = *(undefined1 *)(param_2 + 0xa7);
  *(undefined1 *)((longlong)param_1 + 0x29d) = *(undefined1 *)((longlong)param_2 + 0x29d);
  *(undefined1 *)((longlong)param_1 + 0x29e) = *(undefined1 *)((longlong)param_2 + 0x29e);
  *(undefined1 *)((longlong)param_1 + 0x29f) = *(undefined1 *)((longlong)param_2 + 0x29f);
  *(undefined1 *)(param_1 + 0xa8) = *(undefined1 *)(param_2 + 0xa8);
  *(undefined1 *)((longlong)param_1 + 0x2a1) = *(undefined1 *)((longlong)param_2 + 0x2a1);
  *(undefined1 *)((longlong)param_1 + 0x2a2) = *(undefined1 *)((longlong)param_2 + 0x2a2);
  *(undefined1 *)((longlong)param_1 + 0x2a3) = *(undefined1 *)((longlong)param_2 + 0x2a3);
  *(undefined1 *)(param_1 + 0xa9) = *(undefined1 *)(param_2 + 0xa9);
  *(undefined1 *)((longlong)param_1 + 0x2a5) = *(undefined1 *)((longlong)param_2 + 0x2a5);
  *(undefined1 *)((longlong)param_1 + 0x2a6) = *(undefined1 *)((longlong)param_2 + 0x2a6);
  *(undefined1 *)((longlong)param_1 + 0x2a7) = *(undefined1 *)((longlong)param_2 + 0x2a7);
  *(undefined1 *)(param_1 + 0xaa) = *(undefined1 *)(param_2 + 0xaa);
  *(undefined1 *)((longlong)param_1 + 0x2a9) = *(undefined1 *)((longlong)param_2 + 0x2a9);
  FUN_1808d9890(param_1 + 0xac,param_2 + 0xac);
  return param_1;
}


