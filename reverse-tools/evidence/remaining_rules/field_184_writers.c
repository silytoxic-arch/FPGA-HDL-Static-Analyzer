Target: T-Linter.dll; SHA256: 60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0

===== 0x1800e2b50 1800e2b50 FUN_1800e2b50 =====

/* WARNING: Function: __security_check_cookie replaced with injection: security_check_cookie */

void FUN_1800e2b50(longlong param_1,longlong *param_2)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  uint uVar4;
  longlong lVar5;
  longlong *plVar6;
  longlong *plVar7;
  longlong *plVar8;
  longlong *plVar9;
  longlong *plVar10;
  undefined8 uVar11;
  undefined8 *puVar12;
  undefined1 *_Buf1;
  undefined8 *puVar13;
  size_t _Size;
  undefined1 auStack_1b8 [32];
  undefined8 local_198;
  undefined8 local_190;
  undefined8 local_188;
  undefined8 local_180;
  int iStack_178;
  int iStack_174;
  undefined8 local_170;
  ulonglong local_168;
  undefined1 local_160;
  undefined7 uStack_15f;
  undefined8 local_150;
  ulonglong local_148;
  undefined1 local_140;
  undefined7 uStack_13f;
  undefined8 local_130;
  ulonglong local_128;
  undefined1 local_120;
  undefined7 uStack_11f;
  ulonglong local_110;
  ulonglong local_108;
  undefined8 local_f8;
  longlong local_f0;
  longlong local_e8;
  undefined8 local_e0;
  longlong local_d8;
  longlong local_d0;
  undefined1 local_c8 [16];
  undefined8 local_b8;
  undefined8 local_b0;
  undefined1 local_a8 [16];
  undefined8 local_98;
  undefined8 local_90;
  undefined1 local_88 [16];
  undefined8 local_78;
  undefined8 local_70;
  undefined1 local_68 [16];
  undefined8 local_58;
  undefined8 local_50;
  ulonglong local_48;
  
  local_188 = 0xfffffffffffffffe;
  local_48 = DAT_18131ef10 ^ (ulonglong)auStack_1b8;
  FUN_1800daa20(param_1 + 0x20,*(undefined8 *)(*(longlong *)(param_1 + 0x20) + 8));
  *(longlong *)(*(longlong *)(param_1 + 0x20) + 8) = *(longlong *)(param_1 + 0x20);
  *(undefined8 *)*(undefined8 *)(param_1 + 0x20) = *(undefined8 *)(param_1 + 0x20);
  *(longlong *)(*(longlong *)(param_1 + 0x20) + 0x10) = *(longlong *)(param_1 + 0x20);
  *(undefined8 *)(param_1 + 0x28) = 0;
  *(undefined8 *)(param_1 + 0x30) = 0;
  FUN_1800dab10(param_1 + 0x38,*(undefined8 *)(*(longlong *)(param_1 + 0x38) + 8));
  *(longlong *)(*(longlong *)(param_1 + 0x38) + 8) = *(longlong *)(param_1 + 0x38);
  *(undefined8 *)*(undefined8 *)(param_1 + 0x38) = *(undefined8 *)(param_1 + 0x38);
  *(longlong *)(*(longlong *)(param_1 + 0x38) + 0x10) = *(longlong *)(param_1 + 0x38);
  *(undefined8 *)(param_1 + 0x40) = 0;
  lVar5 = *(longlong *)(param_1 + 0x68);
  while (lVar5 != 0) {
    plVar6 = (longlong *)(param_1 + 0x68);
    *plVar6 = *plVar6 + -1;
    if (*plVar6 == 0) {
      *(undefined8 *)(param_1 + 0x60) = 0;
    }
    lVar5 = *(longlong *)(param_1 + 0x68);
  }
  *(undefined8 *)(param_1 + 0x70) = 0;
  *(undefined8 *)(param_1 + 0x78) = 0;
  *(undefined8 *)(param_1 + 0x80) = 0;
  *(undefined8 *)(param_1 + 0x88) = 0;
  *(undefined8 *)(param_1 + 0x98) = *(undefined8 *)(param_1 + 0x90);
  *(undefined1 *)(param_1 + 0xa8) = 0;
  *(undefined4 *)(param_1 + 0xac) = 0;
  *(undefined8 *)(param_1 + 0xb0) = 0;
  *(undefined8 *)(param_1 + 0xb8) = 0;
  *(undefined2 *)(param_1 + 0xc0) = 0;
  *(undefined4 *)(param_1 + 0xc4) = 0;
  *(undefined1 *)(param_1 + 0x180) = 0;
  *(undefined2 *)(param_1 + 0x182) = 0;
  *(undefined1 *)(param_1 + 0x184) = 0;
  *(undefined8 *)(param_1 + 0x1a0) = 0;
  *(undefined8 *)(param_1 + 0x1a8) = 0;
  *(undefined8 *)(param_1 + 0x1b0) = 0;
  *(undefined8 *)(param_1 + 0x1b8) = 0;
  *(undefined8 *)(param_1 + 0x1c0) = 0;
  *(undefined8 *)(param_1 + 0x1c8) = 0;
  *(undefined8 *)(param_1 + 0x1d0) = 0;
  *(undefined8 *)(param_1 + 0x1d8) = 0;
  *(undefined8 *)(param_1 + 0x1e0) = 0;
  *(undefined8 *)(param_1 + 0x1e8) = 0;
  *(undefined8 *)(param_1 + 0x1f0) = 0;
  *(undefined8 *)(param_1 + 0x1f8) = 0;
  *(undefined8 *)(param_1 + 0x200) = 0;
  *(undefined8 *)(param_1 + 0x208) = 0;
  *(undefined8 *)(param_1 + 0x210) = 0;
  *(undefined8 *)(param_1 + 0x218) = 0;
  *(undefined8 *)(param_1 + 0x220) = 0;
  *(undefined8 *)(param_1 + 0x228) = 0;
  *(undefined8 *)(param_1 + 0x230) = 0;
  *(undefined8 *)(param_1 + 0x238) = 0;
  *(undefined8 *)(param_1 + 0x248) = 0;
  *(undefined8 *)(param_1 + 0x250) = 0;
  *(undefined8 *)(param_1 + 600) = 0;
  *(undefined8 *)(param_1 + 0x260) = 0;
  *(undefined8 *)(param_1 + 0x268) = 0;
  *(undefined8 *)(param_1 + 0x178) = 0;
  plVar6 = (longlong *)(param_1 + 0xd0);
  FUN_1800dab10(plVar6,*(undefined8 *)(*plVar6 + 8));
  *(longlong *)(*plVar6 + 8) = *plVar6;
  *(longlong *)*plVar6 = *plVar6;
  *(longlong *)(*plVar6 + 0x10) = *plVar6;
  *(undefined8 *)(param_1 + 0xd8) = 0;
  FUN_1800dc600(param_1 + 0xe0);
  plVar6 = (longlong *)(param_1 + 0xf0);
  FUN_1800dab10(plVar6,*(undefined8 *)(*plVar6 + 8));
  *(longlong *)(*plVar6 + 8) = *plVar6;
  *(longlong *)*plVar6 = *plVar6;
  *(longlong *)(*plVar6 + 0x10) = *plVar6;
  *(undefined8 *)(param_1 + 0xf8) = 0;
  (**(code **)(*(longlong *)(param_1 + 0x100) + 0x20))();
  (**(code **)(*(longlong *)(param_1 + 0x118) + 0x20))();
  (**(code **)(*(longlong *)(param_1 + 0x130) + 0x20))();
  (**(code **)(*(longlong *)(param_1 + 0x148) + 0x20))();
  (**(code **)(*(longlong *)(param_1 + 0x160) + 0x20))();
  lVar5 = FUN_1800bad20(param_2);
  if (lVar5 != 0) {
    plVar6 = (longlong *)FUN_1800bad20(param_2);
    (**(code **)(*plVar6 + 0x30))(plVar6,&local_120);
    *(undefined1 *)(param_1 + 0xa8) = 0;
    lVar5 = (**(code **)(*param_2 + 0xa8))(param_2);
    if ((undefined8 *)(lVar5 + 8) != &DAT_18137ccc8) {
      FUN_180040550(&DAT_18137ccc8,(undefined8 *)(lVar5 + 8),0,0xffffffffffffffff);
    }
    plVar6 = (longlong *)(**(code **)(*param_2 + 0xb0))(param_2);
    plVar7 = (longlong *)(**(code **)(*param_2 + 0xb0))(param_2);
    plVar8 = (longlong *)(**(code **)(*param_2 + 0xb0))(param_2);
    plVar9 = (longlong *)(**(code **)(*param_2 + 0xa8))(param_2);
    plVar10 = (longlong *)(**(code **)(*param_2 + 0xa8))(param_2);
    lVar5 = (**(code **)(*plVar7 + 8))(plVar7,&local_160);
    uVar11 = *(undefined8 *)(lVar5 + 0x10);
    iVar1 = (**(code **)(*plVar6 + 0x20))(plVar6);
    iVar1 = (int)uVar11 + 1 + iVar1;
    uVar2 = (**(code **)(*plVar8 + 0x18))(plVar8);
    iVar3 = (**(code **)(*plVar9 + 0x20))(plVar9);
    iVar3 = iVar3 + 1;
    DAT_18133c1e8 = (**(code **)(*plVar10 + 0x18))(plVar10);
    local_180 = (undefined8 *)CONCAT44(uVar2,DAT_18133c1e8);
    uRam000000018133c1ec = uVar2;
    iRam000000018133c1f0 = iVar3;
    iRam000000018133c1f4 = iVar1;
    iStack_178 = iVar3;
    iStack_174 = iVar1;
    if (0xf < local_148) {
      FUN_1800409e0(&local_160,CONCAT71(uStack_15f,local_160),local_148 + 1);
    }
    local_148 = 0xf;
    lVar5 = 0;
    local_150 = 0;
    local_160 = 0;
    _Buf1 = &local_120;
    if (0xf < local_108) {
      _Buf1 = (undefined1 *)CONCAT71(uStack_11f,local_120);
    }
    _Size = 6;
    if (local_110 < 6) {
      _Size = local_110;
    }
    uVar4 = 0;
    if (_Size != 0) {
      uVar4 = memcmp(_Buf1,"module",_Size);
    }
    if (uVar4 == 0) {
      if (local_110 < 6) {
        uVar4 = 0xffffffff;
      }
      else {
        uVar4 = (uint)(local_110 != 6);
      }
    }
    if (uVar4 == 0) {
      local_180 = operator_new(0x2c8);
      if (local_180 != (void *)0x0) {
        local_190 = 0;
        local_198 = 0;
        lVar5 = FUN_1808d8dc0(local_180,2,0);
      }
    }
    else {
      local_180 = operator_new(0x2c8);
      if (local_180 != (void *)0x0) {
        local_190 = 0;
        local_198 = 0;
        lVar5 = FUN_1808d8dc0(local_180,2,0);
      }
    }
    plVar6 = (longlong *)(param_1 + 0x178);
    *plVar6 = lVar5;
    FUN_180132830(*(longlong *)(*(longlong *)(param_1 + 0x10) + -8) + 8,plVar6);
    FUN_180132830(param_1 + 8,plVar6);
    *(longlong *)(param_1 + 0xb8) = *plVar6;
    FUN_1800daa20(param_1 + 0x20,*(undefined8 *)(*(longlong *)(param_1 + 0x20) + 8));
    *(longlong *)(*(longlong *)(param_1 + 0x20) + 8) = *(longlong *)(param_1 + 0x20);
    *(undefined8 *)*(undefined8 *)(param_1 + 0x20) = *(undefined8 *)(param_1 + 0x20);
    *(longlong *)(*(longlong *)(param_1 + 0x20) + 0x10) = *(longlong *)(param_1 + 0x20);
    *(undefined8 *)(param_1 + 0x28) = 0;
    *(undefined4 *)(param_1 + 0xac) = 0;
    plVar7 = (longlong *)FUN_1800ba700(param_2);
    plVar7 = (longlong *)(**(code **)(*plVar7 + 0xa8))(plVar7);
    uVar2 = (**(code **)(*plVar7 + 0x18))(plVar7);
    *(undefined4 *)(*plVar6 + 0x210) = uVar2;
    plVar7 = (longlong *)FUN_1800ba700(param_2);
    uVar11 = (**(code **)(*plVar7 + 0x30))(plVar7,&local_140);
    local_168 = 0xf;
    local_170 = 0;
    local_180 = (undefined8 *)((ulonglong)local_180 & 0xffffffffffffff00);
    FUN_180040680(&local_180,&DAT_180e3dc90,1);
    puVar12 = (undefined8 *)FUN_1800d36a0(&local_160,&local_180,uVar11);
    lVar5 = *plVar6;
    puVar13 = (undefined8 *)(lVar5 + 0x48);
    if (puVar13 != puVar12) {
      if (0xf < *(ulonglong *)(lVar5 + 0x60)) {
        FUN_1800409e0(puVar13,*puVar13,*(ulonglong *)(lVar5 + 0x60) + 1);
      }
      *(undefined8 *)(lVar5 + 0x60) = 0xf;
      *(undefined8 *)(lVar5 + 0x58) = 0;
      *(undefined1 *)puVar13 = 0;
      FUN_18004ad80(puVar13,puVar12);
    }
    if (0xf < local_148) {
      FUN_1800409e0(&local_160,CONCAT71(uStack_15f,local_160),local_148 + 1);
    }
    local_148 = 0xf;
    local_150 = 0;
    local_160 = 0;
    if (0xf < local_168) {
      FUN_1800409e0(&local_180,local_180,local_168 + 1);
    }
    local_168 = 0xf;
    local_170 = 0;
    local_180 = (undefined8 *)((ulonglong)local_180 & 0xffffffffffffff00);
    if (0xf < local_128) {
      FUN_1800409e0(&local_140,CONCAT71(uStack_13f,local_140),local_128 + 1);
    }
    local_128 = 0xf;
    local_130 = 0;
    local_140 = 0;
    plVar7 = (longlong *)(**(code **)(*param_2 + 0xa8))(param_2);
    local_f8 = (**(code **)(*plVar7 + 0x18))(plVar7);
    plVar7 = (longlong *)(**(code **)(*param_2 + 0xa8))(param_2);
    local_f0 = (**(code **)(*plVar7 + 0x20))(plVar7);
    local_f0 = local_f0 + 1;
    plVar7 = (longlong *)(**(code **)(*param_2 + 0xa8))(param_2);
    plVar8 = (longlong *)(**(code **)(*param_2 + 0xa8))(param_2);
    lVar5 = (**(code **)(*plVar8 + 8))(plVar8,&local_160);
    lVar5 = *(longlong *)(lVar5 + 0x10);
    local_e8 = (**(code **)(*plVar7 + 0x20))(plVar7);
    local_e8 = local_e8 + lVar5;
    plVar7 = (longlong *)(**(code **)(*param_2 + 0xb0))(param_2);
    local_e0 = (**(code **)(*plVar7 + 0x18))(plVar7);
    plVar7 = (longlong *)(**(code **)(*param_2 + 0xb0))(param_2);
    local_d8 = (**(code **)(*plVar7 + 0x20))(plVar7);
    local_d8 = local_d8 + 1;
    plVar7 = (longlong *)(**(code **)(*param_2 + 0xb0))(param_2);
    plVar8 = (longlong *)(**(code **)(*param_2 + 0xb0))(param_2);
    lVar5 = (**(code **)(*plVar8 + 8))(plVar8,&local_140);
    lVar5 = *(longlong *)(lVar5 + 0x10);
    local_d0 = (**(code **)(*plVar7 + 0x20))(plVar7);
    local_d0 = local_d0 + lVar5;
    lVar5 = (**(code **)(*param_2 + 0xa8))(param_2);
    local_b0 = 0xf;
    local_b8 = 0;
    local_c8[0] = 0;
    FUN_180040550(local_c8,lVar5 + 8,0,0xffffffffffffffff);
    local_90 = 0xf;
    local_98 = 0;
    local_a8[0] = 0;
    FUN_180040680(local_a8,&DAT_180de47b9,0);
    local_70 = 0xf;
    local_78 = 0;
    local_88[0] = 0;
    FUN_180040550(local_88,*plVar6 + 0x48,0,0xffffffffffffffff);
    local_50 = 0xf;
    local_58 = 0;
    local_68[0] = 0;
    if ((&local_f8 < DAT_18133c150) && (DAT_18133c148 <= &local_f8)) {
      lVar5 = (longlong)&local_f8 - (longlong)DAT_18133c148;
      if (DAT_18133c150 == DAT_18133c158) {
        FUN_1800dbb80(&DAT_18133c148,1);
      }
      if (DAT_18133c150 != (undefined8 *)0x0) {
        FUN_1800d7be0(DAT_18133c150,DAT_18133c148 + (lVar5 / 0xb0) * 0x16);
      }
    }
    else {
      if (DAT_18133c150 == DAT_18133c158) {
        FUN_1800dbb80(&DAT_18133c148,1);
      }
      puVar12 = DAT_18133c150;
      local_180 = DAT_18133c150;
      if (DAT_18133c150 != (undefined8 *)0x0) {
        *DAT_18133c150 = local_f8;
        puVar12[1] = local_f0;
        puVar12[2] = local_e8;
        puVar12[3] = local_e0;
        puVar12[4] = local_d8;
        puVar12[5] = local_d0;
        FUN_1800d7720(puVar12 + 6,local_c8);
        FUN_1800d7720(puVar12 + 10,local_a8);
        FUN_1800d7720(puVar12 + 0xe,local_88);
        FUN_1800d7720(puVar12 + 0x12,local_68);
      }
    }
    DAT_18133c150 = DAT_18133c150 + 0x16;
    FUN_1800d8e80(&local_f8);
    if (0xf < local_128) {
      FUN_1800409e0(&local_140,CONCAT71(uStack_13f,local_140),local_128 + 1);
    }
    local_128 = 0xf;
    local_130 = 0;
    local_140 = 0;
    if (0xf < local_148) {
      FUN_1800409e0(&local_160,CONCAT71(uStack_15f,local_160),local_148 + 1);
    }
    local_148 = 0xf;
    local_150 = 0;
    local_160 = 0;
    if (*(longlong *)(param_1 + 0x68) == 0) {
      *(undefined8 *)(param_1 + 0x30) = 0;
    }
    else {
      puVar12 = (undefined8 *)FUN_1801330a0(param_1 + 0x48);
      *(undefined8 *)(param_1 + 0x30) = *puVar12;
      plVar6 = (longlong *)(param_1 + 0x68);
      *plVar6 = *plVar6 + -1;
      if (*plVar6 == 0) {
        *(undefined8 *)(param_1 + 0x60) = 0;
      }
    }
    FUN_1800bba80(param_2);
    if (0xf < local_108) {
      FUN_1800409e0(&local_120,CONCAT71(uStack_11f,local_120),local_108 + 1);
    }
  }
  return;
}



===== 0x1800e7d90 1800e7d90 FUN_1800e7d90 =====

void FUN_1800e7d90(longlong param_1)

{
  *(undefined1 *)(param_1 + 0x184) = 1;
  return;
}



===== 0x180123f90 180123f90 FUN_180123f90 =====

void FUN_180123f90(longlong param_1)

{
  *(undefined1 *)(param_1 + 0x184) = 0;
  return;
}



===== 0x1801ad560 1801ad560 FUN_1801ad560 =====

void FUN_1801ad560(void *param_1)

{
  longlong *plVar1;
  int iVar2;
  longlong lVar3;
  void **ppvVar4;
  void **ppvVar5;
  bool bVar6;
  bool bVar7;
  bool bVar8;
  void *pvVar9;
  void *pvVar10;
  undefined8 uVar11;
  void *pvVar12;
  void *pvVar13;
  longlong *plVar14;
  longlong *plVar15;
  longlong *_Dst;
  longlong *_Src;
  undefined1 local_res8;
  int local_res18;
  void *local_res20;
  void *local_c8;
  longlong *local_c0;
  longlong local_b8;
  void *local_b0;
  void *local_a8;
  void *local_a0;
  undefined8 local_98;
  void *local_90;
  void *local_88;
  void *local_80;
  void *local_78;
  void *local_70;
  undefined1 local_60 [32];
  
  if (param_1 != (void *)0x0) {
    local_98 = 0xfffffffffffffffe;
    local_res8 = SUB81(param_1,0);
    do {
      pvVar13 = (void *)0x0;
      if ((*(longlong *)((longlong)param_1 + 0x10) - *(longlong *)((longlong)param_1 + 8) &
          0xfffffffffffffff8U) == 0) {
        return;
      }
      local_90 = operator_new(0x2c8);
      pvVar9 = pvVar13;
      if (local_90 != (void *)0x0) {
        pvVar9 = (void *)FUN_1808d8dc0(local_90,0x54,0,0,0,0);
      }
      local_a8 = pvVar9;
      pvVar10 = operator_new(0x2c8);
      pvVar12 = pvVar13;
      local_88 = pvVar10;
      if (pvVar10 != (void *)0x0) {
        uVar11 = FUN_1808e3a40(1,0,1);
        pvVar12 = (void *)FUN_1808d8dc0(pvVar10,0x58,uVar11,pvVar9,0,0);
      }
      local_a0 = pvVar12;
      local_80 = operator_new(0x2c8);
      pvVar9 = pvVar13;
      if (local_80 != (void *)0x0) {
        pvVar9 = (void *)FUN_1808d8dc0(local_80,0x54,0,0,0,0);
      }
      bVar7 = false;
      bVar8 = false;
      local_res20 = pvVar9;
      pvVar10 = operator_new(0x2c8);
      local_78 = pvVar10;
      if (pvVar10 == (void *)0x0) {
        local_c8 = (void *)0x0;
      }
      else {
        local_70 = operator_new(0x2c8);
        if (local_70 != (void *)0x0) {
          pvVar13 = (void *)FUN_1808d8dc0(local_70,0x5b,0,0,0,0);
        }
        local_c8 = (void *)FUN_1808d8dc0(pvVar10,0x58,pvVar13,0,0,0);
      }
      local_b8 = 0;
      local_res18 = 0;
      _Dst = *(longlong **)((longlong)param_1 + 8);
      local_b0 = local_c8;
      if (_Dst == *(longlong **)((longlong)param_1 + 0x10)) {
LAB_1801adbce:
        if (local_c8 != (void *)0x0) {
          FUN_1808d9570(local_c8);
          free(local_c8);
        }
      }
      else {
        _Src = _Dst + 1;
        local_c0 = _Src;
        do {
          pvVar13 = local_a8;
          lVar3 = *_Dst;
          if (*(int *)(lVar3 + 4) == 0xab) {
            local_res18 = local_res18 + 1;
            if ((1 < local_res18) && (pvVar9 != (void *)0x0)) {
              *(undefined4 *)((longlong)pvVar9 + 0x214) = *(undefined4 *)(lVar3 + 0x214);
              *(undefined4 *)((longlong)pvVar9 + 0x200) = *(undefined4 *)(*_Dst + 0x214);
              FUN_1801acb50((undefined8 *)((longlong)pvVar9 + 8),
                            *(undefined8 *)((longlong)pvVar9 + 8),_Dst,
                            *(undefined8 *)((longlong)param_1 + 0x10),local_res8);
              if (local_b8 != 0) {
                FUN_180132830(local_b8 + 8,&local_b0);
                local_c8 = local_b0;
                bVar7 = true;
                FUN_180132830((longlong)local_b0 + 8,&local_res20);
                *(undefined4 *)((longlong)local_c8 + 0x214) = *(undefined4 *)(*_Dst + 0x214);
                *(undefined4 *)((longlong)local_c8 + 0x200) = *(undefined4 *)(*_Dst + 0x214);
                *(undefined4 *)(**(longlong **)((longlong)local_c8 + 8) + 0x214) =
                     *(undefined4 *)(*_Dst + 0x214);
                *(undefined4 *)(**(longlong **)((longlong)local_c8 + 8) + 0x200) =
                     *(undefined4 *)(*_Dst + 0x214);
                pvVar9 = local_res20;
              }
              FUN_1801c58e0((longlong)param_1 + 8);
              break;
            }
            *(undefined4 *)(lVar3 + 4) = 0x57;
            *(undefined1 *)(*_Dst + 0x184) = 1;
            bVar6 = false;
            lVar3 = *_Dst;
            _Src = local_c0;
            if ((*(longlong *)(lVar3 + 0x10) - *(longlong *)(lVar3 + 8) >> 3 != 0) &&
               (plVar14 = *(longlong **)(lVar3 + 8), plVar14 != *(longlong **)(lVar3 + 0x10))) {
              plVar15 = plVar14 + 1;
              do {
                iVar2 = *(int *)(*plVar14 + 4);
                if (iVar2 == 0x58) {
                  bVar6 = true;
                  *(undefined4 *)((longlong)pvVar12 + 0x214) = *(undefined4 *)(*plVar14 + 0x214);
                  *(undefined4 *)((longlong)pvVar12 + 0x200) = *(undefined4 *)(*plVar14 + 0x214);
                  *(undefined4 *)(**(longlong **)((longlong)pvVar12 + 8) + 0x214) =
                       *(undefined4 *)(*plVar14 + 0x214);
                  *(undefined4 *)(**(longlong **)((longlong)pvVar12 + 8) + 0x200) =
                       *(undefined4 *)(*plVar14 + 0x214);
                  *(undefined4 *)(*plVar14 + 4) = 0x2d;
                  plVar14 = plVar14 + 1;
                  plVar15 = plVar15 + 1;
                }
                else if ((iVar2 == 0x54) || (iVar2 == 99)) {
                  FUN_180132830((longlong)pvVar13 + 8,plVar14);
                  *(undefined4 *)((longlong)pvVar13 + 0x214) = *(undefined4 *)(*plVar14 + 0x214);
                  *(undefined4 *)((longlong)pvVar13 + 0x200) = *(undefined4 *)(*plVar14 + 0x214);
                  lVar3 = *_Dst;
                  memmove(plVar14,plVar15,
                          *(longlong *)(lVar3 + 0x10) - (longlong)plVar15 & 0xfffffffffffffff8);
                  plVar1 = (longlong *)(lVar3 + 0x10);
                  *plVar1 = *plVar1 + -8;
                }
                else {
                  plVar14 = plVar14 + 1;
                  plVar15 = plVar15 + 1;
                }
              } while (plVar14 != *(longlong **)(*_Dst + 0x10));
              local_res8 = SUB81(param_1,0);
              bVar7 = bVar8;
              if (bVar6) {
                FUN_1801a6990();
                local_b8 = *_Dst;
              }
            }
LAB_1801adac6:
            _Dst = _Dst + 1;
            _Src = _Src + 1;
            local_c0 = _Src;
          }
          else {
            if (*(int *)(lVar3 + 4) != 0xac) {
              if (bVar7) {
                return;
              }
              if (local_c8 == (void *)0x0) {
                return;
              }
              FUN_1808d9570(local_c8);
              free(local_c8);
              return;
            }
            if (local_res18 != 1) goto LAB_1801adac6;
            if (*(longlong *)(lVar3 + 0x10) - *(longlong *)(lVar3 + 8) >> 3 != 0) {
              plVar14 = *(longlong **)(lVar3 + 8);
              if (plVar14 != *(longlong **)(lVar3 + 0x10)) {
                plVar15 = plVar14 + 1;
                do {
                  iVar2 = *(int *)(*plVar14 + 4);
                  if ((iVar2 == 0x54) || (iVar2 == 99)) {
                    *(undefined4 *)((longlong)pvVar9 + 0x214) = *(undefined4 *)(*plVar14 + 0x214);
                    *(undefined4 *)((longlong)pvVar9 + 0x200) = *(undefined4 *)(*plVar14 + 0x214);
                    FUN_180132830((longlong)pvVar9 + 8,plVar14);
                    lVar3 = *_Dst;
                    memmove(plVar14,plVar15,
                            *(longlong *)(lVar3 + 0x10) - (longlong)plVar15 & 0xfffffffffffffff8);
                    plVar1 = (longlong *)(lVar3 + 0x10);
                    *plVar1 = *plVar1 + -8;
                  }
                  else {
                    plVar14 = plVar14 + 1;
                    plVar15 = plVar15 + 1;
                  }
                } while (plVar14 != *(longlong **)(*_Dst + 0x10));
              }
              ppvVar4 = *(void ***)((longlong)local_c8 + 0x10);
              if ((&local_res20 < ppvVar4) &&
                 (ppvVar5 = *(void ***)((longlong)local_c8 + 8), ppvVar5 <= &local_res20)) {
                if (ppvVar4 == *(void ***)((longlong)local_c8 + 0x18)) {
                  FUN_1800db6d0((longlong)local_c8 + 8,1);
                }
                if (*(undefined8 **)((longlong)local_c8 + 0x10) != (undefined8 *)0x0) {
                  **(undefined8 **)((longlong)local_c8 + 0x10) =
                       *(undefined8 *)
                        (*(longlong *)((longlong)local_c8 + 8) +
                        ((longlong)&local_res20 - (longlong)ppvVar5 >> 3) * 8);
                }
              }
              else {
                if (ppvVar4 == *(void ***)((longlong)local_c8 + 0x18)) {
                  FUN_1800db6d0((longlong)local_c8 + 8,1);
                }
                if (*(longlong **)((longlong)local_c8 + 0x10) != (longlong *)0x0) {
                  **(longlong **)((longlong)local_c8 + 0x10) = (longlong)pvVar9;
                }
              }
              *(longlong *)((longlong)local_c8 + 0x10) =
                   *(longlong *)((longlong)local_c8 + 0x10) + 8;
            }
            *(undefined1 *)((longlong)local_c8 + 0x185) = 1;
            *(undefined4 *)((longlong)local_c8 + 0x214) = *(undefined4 *)(*_Dst + 0x214);
            *(undefined4 *)((longlong)local_c8 + 0x200) = *(undefined4 *)(*_Dst + 0x214);
            *(undefined4 *)(**(longlong **)((longlong)local_c8 + 8) + 0x214) =
                 *(undefined4 *)(*_Dst + 0x214);
            *(undefined4 *)(**(longlong **)((longlong)local_c8 + 8) + 0x200) =
                 *(undefined4 *)(*_Dst + 0x214);
            FUN_1801a6990((longlong *)(local_b8 + 8),local_60,*(longlong *)(local_b8 + 8) + 0x10,1,
                          &local_b0);
            bVar7 = true;
            bVar8 = true;
            memmove(_Dst,_Src,
                    *(longlong *)((longlong)param_1 + 0x10) - (longlong)_Src & 0xfffffffffffffff8);
            *(longlong *)((longlong)param_1 + 0x10) = *(longlong *)((longlong)param_1 + 0x10) + -8;
          }
        } while (_Dst != *(longlong **)((longlong)param_1 + 0x10));
        if (!bVar7) goto LAB_1801adbce;
      }
      param_1 = pvVar9;
      if ((*(longlong *)((longlong)pvVar9 + 0x10) - *(longlong *)((longlong)pvVar9 + 8) &
          0xfffffffffffffff8U) == 0) {
        if (pvVar9 != (void *)0x0) {
          FUN_1808d9570(pvVar9);
          free(pvVar9);
        }
        param_1 = (void *)0x0;
      }
    } while (param_1 != (void *)0x0);
  }
  return;
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


