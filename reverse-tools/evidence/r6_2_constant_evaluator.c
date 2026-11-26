Target: T-Linter.dll; SHA256: 60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0

=== 0x1804b78d0 FUN_1804b78d0 ===
completed=true; error=

undefined8 FUN_1804b78d0(longlong param_1,longlong param_2,uint *param_3)

{
  int iVar1;
  char cVar2;
  longlong lVar3;
  uint *puVar4;
  uint local_res10 [4];
  int local_res20 [2];
  
  iVar1 = *(int *)(param_2 + 4);
  if (iVar1 == 0x10) {
    if (*(char *)(param_2 + 0x109) != '\0') {
      *param_3 = *(uint *)(param_2 + 0xbc);
      return 1;
    }
  }
  else if (iVar1 == 0x43) {
    local_res10[0] = 0xffffffff;
    FUN_1804b78d0(param_1,**(undefined8 **)(param_2 + 8),local_res10);
    if (local_res10[0] != 0) {
      *param_3 = local_res10[0];
      return 1;
    }
  }
  else if (iVar1 == 0x44) {
    local_res10[0] = 0xffffffff;
    FUN_1804b78d0(param_1,**(undefined8 **)(param_2 + 8),local_res10);
    if (local_res10[0] != 0) {
      *param_3 = -local_res10[0];
      return 1;
    }
  }
  else if (iVar1 == 0x13) {
    lVar3 = FUN_180676aa0(param_1 + 0x1a8,param_2 + 0x48);
    if (lVar3 != 0) {
      puVar4 = (uint *)FUN_1800d95e0(param_1 + 0x1a8,param_2 + 0x48);
      *param_3 = *puVar4;
      return 1;
    }
  }
  else if ((iVar1 == 0x3c) &&
          ((*(longlong *)(param_2 + 0x10) - *(longlong *)(param_2 + 8) & 0xfffffffffffffff8U) ==
           0x10)) {
    local_res10[0] = 0xffffffff;
    local_res20[0] = -1;
    cVar2 = FUN_1804b78d0(param_1,**(undefined8 **)(param_2 + 8),local_res10);
    if (cVar2 != '\0') {
      cVar2 = FUN_1804b78d0(param_1,*(undefined8 *)(*(longlong *)(param_2 + 8) + 8),local_res20);
      if (cVar2 != '\0') {
        *param_3 = local_res10[0] + local_res20[0];
        return 1;
      }
    }
  }
  else if (iVar1 == 0x3d) {
    local_res10[0] = 0xffffffff;
    local_res20[0] = -1;
    cVar2 = FUN_1804b78d0(param_1,**(undefined8 **)(param_2 + 8),local_res10);
    if (cVar2 != '\0') {
      cVar2 = FUN_1804b78d0(param_1,*(undefined8 *)(*(longlong *)(param_2 + 8) + 8),local_res20);
      if (cVar2 != '\0') {
        *param_3 = local_res10[0] - local_res20[0];
        return 1;
      }
    }
  }
  else if (iVar1 == 0x3e) {
    local_res20[0] = -1;
    local_res10[0] = 0xffffffff;
    cVar2 = FUN_1804b78d0(param_1,**(undefined8 **)(param_2 + 8),local_res20);
    if (cVar2 != '\0') {
      cVar2 = FUN_1804b78d0(param_1,*(undefined8 *)(*(longlong *)(param_2 + 8) + 8),local_res10);
      if (cVar2 != '\0') {
        *param_3 = local_res10[0] * local_res20[0];
        return 1;
      }
    }
  }
  else if (iVar1 == 0x3f) {
    local_res10[0] = 0xffffffff;
    local_res20[0] = -1;
    cVar2 = FUN_1804b78d0(param_1,**(undefined8 **)(param_2 + 8),local_res10);
    if (cVar2 != '\0') {
      cVar2 = FUN_1804b78d0(param_1,*(undefined8 *)(*(longlong *)(param_2 + 8) + 8),local_res20);
      if (cVar2 != '\0') {
        *param_3 = (int)local_res10[0] / local_res20[0];
        return 1;
      }
    }
  }
  return 0;
}


