Target: T-Linter.dll; SHA256: 60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0

===== 0x1806097d0 1806097d0 FUN_1806097d0 =====

void FUN_1806097d0(longlong param_1)

{
  code *pcVar1;
  longlong lVar2;
  ulonglong uVar3;
  longlong lVar4;
  undefined8 *puVar5;
  ulonglong uVar6;
  ulonglong uVar7;
  ulonglong uVar8;
  longlong local_res8;
  
  uVar7 = 0;
  local_res8 = param_1;
  if (*(int *)(param_1 + 4) == 0xb0) {
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
        uVar6 = lVar2 + 1;
        uVar8 = (longlong)DAT_18135a990 - (longlong)DAT_18135a980 >> 3;
        uVar3 = (uVar8 >> 1) + uVar8;
        if (0x1fffffffffffffff - (uVar8 >> 1) < uVar8) {
          uVar3 = uVar7;
        }
        if (uVar3 < uVar6) {
          uVar3 = uVar6;
        }
        FUN_1800daf20(&DAT_18135a980,uVar3);
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
        uVar6 = lVar4 + 1;
        uVar8 = (longlong)DAT_18135a990 - (longlong)DAT_18135a980 >> 3;
        uVar3 = (uVar8 >> 1) + uVar8;
        if (0x1fffffffffffffff - (uVar8 >> 1) < uVar8) {
          uVar3 = uVar7;
        }
        if (uVar3 < uVar6) {
          uVar3 = uVar6;
        }
        FUN_1800daf20(&DAT_18135a980,uVar3);
      }
      if (DAT_18135a988 != (longlong *)0x0) {
        *DAT_18135a988 = param_1;
      }
    }
    DAT_18135a988 = DAT_18135a988 + 1;
  }
  if ((*(int *)(param_1 + 4) == 0x54) &&
     ((*(longlong *)(param_1 + 0x10) - *(longlong *)(param_1 + 8) & 0xfffffffffffffff8U) == 0)) {
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
        uVar6 = lVar2 + 1;
        uVar8 = (longlong)DAT_18135a990 - (longlong)DAT_18135a980 >> 3;
        uVar3 = (uVar8 >> 1) + uVar8;
        if (0x1fffffffffffffff - (uVar8 >> 1) < uVar8) {
          uVar3 = uVar7;
        }
        if (uVar3 < uVar6) {
          uVar3 = uVar6;
        }
        FUN_1800daf20(&DAT_18135a980,uVar3);
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
        uVar6 = lVar4 + 1;
        uVar8 = (longlong)DAT_18135a990 - (longlong)DAT_18135a980 >> 3;
        uVar3 = (uVar8 >> 1) + uVar8;
        if (0x1fffffffffffffff - (uVar8 >> 1) < uVar8) {
          uVar3 = uVar7;
        }
        if (uVar3 < uVar6) {
          uVar3 = uVar6;
        }
        FUN_1800daf20(&DAT_18135a980,uVar3);
      }
      if (DAT_18135a988 != (longlong *)0x0) {
        *DAT_18135a988 = param_1;
      }
    }
    DAT_18135a988 = DAT_18135a988 + 1;
  }
  puVar5 = *(undefined8 **)(param_1 + 8);
  uVar6 = (ulonglong)((longlong)*(undefined8 **)(param_1 + 0x10) + (7 - (longlong)puVar5)) >> 3;
  if (*(undefined8 **)(param_1 + 0x10) < puVar5) {
    uVar6 = uVar7;
  }
  if (uVar6 != 0) {
    do {
      FUN_1806097d0(*puVar5);
      uVar7 = uVar7 + 1;
      puVar5 = puVar5 + 1;
    } while (uVar7 != uVar6);
  }
  return;
}


