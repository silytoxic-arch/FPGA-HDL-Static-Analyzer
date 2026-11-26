Target: T-Linter.dll; SHA256: 60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0

===== 0x180609b30 180609b30 FUN_180609b30 =====

void FUN_180609b30(longlong param_1)

{
  code *pcVar1;
  longlong lVar2;
  ulonglong uVar3;
  longlong lVar4;
  ulonglong uVar5;
  ulonglong uVar6;
  ulonglong uVar7;
  longlong local_res8;
  
  if (param_1 != 0) {
    uVar6 = 0;
    local_res8 = param_1;
    if ((*(int *)(param_1 + 4) == 0x52) &&
       (*(longlong *)(param_1 + 0x10) - *(longlong *)(param_1 + 8) >> 3 != 0)) {
      FUN_1806097d0();
      lVar4 = **(longlong **)(param_1 + 8);
      if ((*(int *)(lVar4 + 4) == 0x54) &&
         (((*(longlong *)(lVar4 + 0x10) - *(longlong *)(lVar4 + 8) & 0xfffffffffffffff8U) == 8 &&
          (*(int *)(**(longlong **)(lVar4 + 8) + 4) == 0x67)))) {
        if ((&local_res8 < DAT_18135a988) && (DAT_18135a980 <= &local_res8)) {
          lVar4 = (longlong)&local_res8 - (longlong)DAT_18135a980;
          if ((DAT_18135a988 == DAT_18135a990) &&
             ((longlong)DAT_18135a990 - (longlong)DAT_18135a988 >> 3 == 0)) {
            lVar2 = (longlong)DAT_18135a988 - (longlong)DAT_18135a980 >> 3;
            if (lVar2 == 0x1fffffffffffffff) {
              std::_Xlength_error("vector<T> too long");
              pcVar1 = (code *)swi(3);
              (*pcVar1)();
              return;
            }
            uVar3 = lVar2 + 1;
            uVar7 = (longlong)DAT_18135a990 - (longlong)DAT_18135a980 >> 3;
            uVar5 = (uVar7 >> 1) + uVar7;
            if (0x1fffffffffffffff - (uVar7 >> 1) < uVar7) {
              uVar5 = uVar6;
            }
            if (uVar5 < uVar3) {
              uVar5 = uVar3;
            }
            FUN_1800daf20(&DAT_18135a980,uVar5);
          }
          if (DAT_18135a988 != (longlong *)0x0) {
            *DAT_18135a988 = DAT_18135a980[lVar4 >> 3];
          }
        }
        else {
          if ((DAT_18135a988 == DAT_18135a990) &&
             ((longlong)DAT_18135a990 - (longlong)DAT_18135a988 >> 3 == 0)) {
            lVar4 = (longlong)DAT_18135a988 - (longlong)DAT_18135a980 >> 3;
            if (lVar4 == 0x1fffffffffffffff) {
              std::_Xlength_error("vector<T> too long");
              pcVar1 = (code *)swi(3);
              (*pcVar1)();
              return;
            }
            uVar3 = lVar4 + 1;
            uVar7 = (longlong)DAT_18135a990 - (longlong)DAT_18135a980 >> 3;
            uVar5 = (uVar7 >> 1) + uVar7;
            if (0x1fffffffffffffff - (uVar7 >> 1) < uVar7) {
              uVar5 = uVar6;
            }
            if (uVar5 < uVar3) {
              uVar5 = uVar3;
            }
            FUN_1800daf20(&DAT_18135a980,uVar5);
          }
          if (DAT_18135a988 != (longlong *)0x0) {
            *DAT_18135a988 = param_1;
          }
        }
        DAT_18135a988 = DAT_18135a988 + 1;
      }
    }
    if (*(longlong *)(param_1 + 0x10) - *(longlong *)(param_1 + 8) >> 3 != 0) {
      do {
        FUN_180609b30(*(undefined8 *)(*(longlong *)(param_1 + 8) + uVar6 * 8));
        uVar6 = uVar6 + 1;
      } while (uVar6 < (ulonglong)(*(longlong *)(param_1 + 0x10) - *(longlong *)(param_1 + 8) >> 3))
      ;
    }
  }
  return;
}


