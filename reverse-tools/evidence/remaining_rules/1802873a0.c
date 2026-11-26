// Static decompiler evidence; not original source or runtime observation.
// Target: T-Linter.dll; SHA256: 60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0; entry: 1802873a0

void FUN_1802873a0(longlong *param_1)

{
  longlong lVar1;
  int iVar2;
  int iVar3;
  longlong lVar4;
  undefined4 auStackX_8 [8];
  
  param_1[1] = *param_1;
  auStackX_8[0] = 0xffffffff;
  iVar2 = FUN_180047790((int)(param_1[5] - param_1[3] >> 3) * 3);
  FUN_180048300(param_1,(longlong)iVar2,auStackX_8);
  iVar2 = 0;
  if (0 < (int)(param_1[4] - param_1[3] >> 3)) {
    lVar4 = 0;
    do {
      iVar3 = 0;
      lVar1 = *param_1;
      if (lVar1 != param_1[1]) {
        iVar3 = (int)((ulonglong)*(byte *)(lVar4 + param_1[3]) %
                     (param_1[1] - lVar1 >> 2 & 0xffffffffU));
      }
      *(undefined4 *)(lVar4 + 4 + param_1[3]) = *(undefined4 *)((longlong)iVar3 * 4 + lVar1);
      lVar4 = lVar4 + 8;
      *(int *)((longlong)iVar3 * 4 + *param_1) = iVar2;
      iVar2 = iVar2 + 1;
    } while (iVar2 < (int)(param_1[4] - param_1[3] >> 3));
  }
  return;
}

