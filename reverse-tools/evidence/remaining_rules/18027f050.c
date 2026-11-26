// Static decompiler evidence; not original source or runtime observation.
// Target: T-Linter.dll; SHA256: 60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0; entry: 18027f050

ulonglong * FUN_18027f050(ulonglong *param_1,ulonglong *param_2)

{
  undefined8 *puVar1;
  char cVar2;
  ulonglong uVar3;
  undefined8 *puVar4;
  ulonglong uVar5;
  undefined8 *puVar6;
  undefined1 uStackX_8;
  
  if (param_1 != param_2) {
    puVar1 = (undefined8 *)param_2[1];
    puVar6 = (undefined8 *)*param_2;
    if (puVar6 == puVar1) {
      param_1[1] = *param_1;
    }
    else {
      uVar3 = (longlong)puVar1 - (longlong)puVar6 >> 3;
      puVar4 = (undefined8 *)*param_1;
      uVar5 = (longlong)(param_1[1] - (longlong)puVar4) >> 3;
      if (uVar5 < uVar3) {
        uStackX_8 = SUB81(param_1,0);
        if ((ulonglong)((longlong)(param_1[2] - (longlong)puVar4) >> 3) < uVar3) {
          if (puVar4 != (undefined8 *)0x0) {
            FUN_18043ae30();
          }
          cVar2 = FUN_180253fe0(param_1,(longlong)(param_2[1] - *param_2) >> 3);
          if (cVar2 != '\0') {
            uVar3 = FUN_18027ce30(*param_2,param_2[1],*param_1,param_1,uStackX_8);
            param_1[1] = uVar3;
          }
        }
        else {
          func_0x0001801a4e40(puVar6,puVar6 + uVar5,puVar4);
          uVar3 = FUN_18027ce30(puVar6 + uVar5,param_2[1],param_1[1],param_1,uStackX_8);
          param_1[1] = uVar3;
        }
      }
      else {
        uVar5 = 0;
        uVar3 = (ulonglong)((longlong)puVar1 + (7 - (longlong)puVar6)) >> 3;
        if (puVar1 < puVar6) {
          uVar3 = uVar5;
        }
        if (uVar3 != 0) {
          do {
            *puVar4 = *puVar6;
            puVar4 = puVar4 + 1;
            puVar6 = puVar6 + 1;
            uVar5 = uVar5 + 1;
          } while (uVar5 != uVar3);
        }
        param_1[1] = *param_1 + ((longlong)(param_2[1] - *param_2) >> 3) * 8;
      }
    }
  }
  return param_1;
}

