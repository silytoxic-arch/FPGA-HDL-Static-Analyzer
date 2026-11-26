// Static decompiler evidence; not original source or runtime observation.
// Target: T-Linter.dll; SHA256: 60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0; entry: 180557340

/* WARNING: Removing unreachable block (ram,0x000180557479) */
/* WARNING: Removing unreachable block (ram,0x000180557480) */
/* WARNING: Removing unreachable block (ram,0x000180557489) */
/* WARNING: Removing unreachable block (ram,0x0001805574ae) */
/* WARNING: Removing unreachable block (ram,0x0001805574b8) */
/* WARNING: Removing unreachable block (ram,0x0001805574be) */
/* WARNING: Removing unreachable block (ram,0x0001805574cb) */
/* WARNING: Removing unreachable block (ram,0x000180557493) */
/* WARNING: Removing unreachable block (ram,0x0001805574a0) */
/* WARNING: Removing unreachable block (ram,0x0001805574ac) */
/* WARNING: Removing unreachable block (ram,0x0001805574ce) */
/* WARNING: Removing unreachable block (ram,0x000180557656) */
/* WARNING: Removing unreachable block (ram,0x000180557bda) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_180557340(longlong param_1)

{
  int *piVar1;
  longlong *plVar2;
  longlong *plVar3;
  code *pcVar4;
  ulonglong uVar5;
  longlong *plVar6;
  int iVar7;
  longlong *plVar8;
  char cVar9;
  undefined4 uVar10;
  char *pcVar11;
  undefined8 *puVar12;
  undefined1 *puVar13;
  undefined1 *puVar14;
  longlong lVar15;
  longlong *plVar16;
  ulonglong uVar17;
  longlong *plVar18;
  int iVar19;
  char *pcVar20;
  undefined1 *puVar21;
  int iVar22;
  uint uVar23;
  undefined1 auStack_288 [32];
  undefined8 *puStack_268;
  undefined8 uStack_260;
  int aiStack_258 [2];
  ulonglong uStack_250;
  longlong lStack_248;
  longlong lStack_240;
  int *piStack_238;
  undefined8 uStack_230;
  undefined1 auStack_220 [8];
  undefined1 auStack_218 [56];
  undefined8 uStack_1e0;
  undefined8 uStack_1d8;
  undefined8 uStack_1d0;
  longlong lStack_1c8;
  longlong lStack_1c0;
  undefined8 uStack_1b8;
  undefined1 uStack_1a8;
  undefined7 uStack_1a7;
  ulonglong uStack_198;
  ulonglong uStack_190;
  undefined1 uStack_188;
  undefined7 uStack_187;
  ulonglong uStack_178;
  ulonglong uStack_170;
  undefined1 uStack_168;
  undefined7 uStack_167;
  undefined8 uStack_158;
  ulonglong uStack_150;
  undefined1 uStack_148;
  undefined7 uStack_147;
  undefined8 uStack_138;
  ulonglong uStack_130;
  undefined1 uStack_128;
  undefined7 uStack_127;
  undefined8 uStack_118;
  ulonglong uStack_110;
  undefined1 auStack_108 [16];
  undefined8 uStack_f8;
  undefined8 uStack_f0;
  undefined1 uStack_e8;
  undefined7 uStack_e7;
  ulonglong uStack_d8;
  ulonglong uStack_d0;
  undefined1 auStack_c8 [16];
  undefined8 uStack_b8;
  undefined8 uStack_b0;
  undefined4 uStack_a8;
  undefined1 auStack_a0 [16];
  undefined8 uStack_90;
  undefined8 uStack_88;
  undefined1 auStack_80 [16];
  undefined8 uStack_70;
  undefined8 uStack_68;
  byte bStack_58;
  undefined7 uStack_57;
  undefined8 uStack_48;
  ulonglong uStack_40;
  ulonglong uStack_38;
  
  plVar6 = plRam000000018136fd78;
  uStack_230 = 0xfffffffffffffffe;
  uStack_38 = _DAT_18131ef10 ^ (ulonglong)auStack_288;
  lStack_248 = param_1;
  plVar18 = plRam000000018136fd70;
  do {
    if (plVar18 == plVar6) {
code_r0x000180557bef:
      FUN_180bc5ed0(uStack_38 ^ (ulonglong)auStack_288);
      return;
    }
    lStack_240 = *plVar18;
    if ((lStack_240 != 0) &&
       (lVar15 = *(longlong *)(lStack_240 + 0x200) - *(longlong *)(lStack_240 + 0x1f8),
       lVar15 = lVar15 / 6 + (lVar15 >> 0x3f),
       uVar23 = ((int)(lVar15 >> 2) - (int)(lVar15 >> 0x3f)) - 1, uStack_250 = (ulonglong)uVar23,
       uVar23 != 0xffffffff)) {
code_r0x000180557400:
      iVar22 = 0;
      iVar19 = (int)uStack_250;
      lVar15 = *(longlong *)(lStack_240 + 0x1f8) + (longlong)iVar19 * 0x18;
      puStack_268 = (undefined8 *)0x0;
      uStack_260 = 0;
      puStack_268 = (undefined8 *)FUN_180280640(&puStack_268);
      if (1 < *(ulonglong *)(*(longlong *)(lVar15 + 8) + 0x120)) {
        plVar2 = *(longlong **)(*(longlong *)(lVar15 + 8) + 0x118);
        plVar16 = (longlong *)*plVar2;
        if (plVar16 != plVar2) {
          do {
            FUN_180996780(auStack_218,plVar16 + 4);
            piStack_238 = (int *)&uStack_1e0;
            uStack_1e0 = 0;
            uStack_1d8 = 0;
            uStack_1d0 = 0;
            lStack_1c8 = 0;
            lStack_1c0 = 0;
            uStack_1b8 = 0;
            FUN_18027f050(&lStack_1c8,plVar16 + 0xe);
            FUN_1802873a0(&uStack_1e0);
            iVar7 = _DAT_18137deac;
            iVar19 = (int)(lStack_1c0 - lStack_1c8 >> 3);
            do {
              iVar19 = iVar19 + -1;
              if (iVar19 == -1) goto code_r0x000180557585;
            } while (*(char *)(lStack_1c8 + (longlong)iVar19 * 8) != '\x01');
            iVar22 = iVar22 + 1;
code_r0x000180557585:
            if (1 < iVar22) {
              piStack_238 = aiStack_258;
              if (_DAT_18137deac != 0) {
                piVar1 = (int *)(_DAT_18137dd08 + (longlong)_DAT_18137deac * 4);
                *piVar1 = *piVar1 + 1;
              }
              aiStack_258[0] = iVar7;
              cVar9 = FUN_1809bc3f0(*(undefined8 *)(lVar15 + 8),aiStack_258);
              if (cVar9 == '\0') goto code_r0x000180557b4e;
              FUN_18028c520(*(undefined8 *)(lVar15 + 8),&uStack_1a8);
              if (uStack_198 == 0) goto code_r0x000180557b24;
              puVar14 = &uStack_1a8;
              if (0xf < uStack_190) {
                puVar14 = (undefined1 *)CONCAT71(uStack_1a7,uStack_1a8);
              }
              pcVar20 = puVar14 + (uStack_198 - 1);
              goto code_r0x000180557680;
            }
            FUN_18027e300(&uStack_1e0);
            FUN_18003f330(auStack_218);
            if (*(char *)((longlong)plVar16 + 0x19) == '\0') {
              plVar3 = (longlong *)plVar16[2];
              if (*(char *)((longlong)plVar3 + 0x19) == '\0') {
                cVar9 = *(char *)(*plVar3 + 0x19);
                plVar16 = plVar3;
                plVar3 = (longlong *)*plVar3;
                while (cVar9 == '\0') {
                  cVar9 = *(char *)(*plVar3 + 0x19);
                  plVar16 = plVar3;
                  plVar3 = (longlong *)*plVar3;
                }
              }
              else {
                cVar9 = *(char *)(plVar16[1] + 0x19);
                plVar8 = (longlong *)plVar16[1];
                plVar3 = plVar16;
                while ((plVar16 = plVar8, cVar9 == '\0' && (plVar3 == (longlong *)plVar16[2]))) {
                  cVar9 = *(char *)(plVar16[1] + 0x19);
                  plVar8 = (longlong *)plVar16[1];
                  plVar3 = plVar16;
                }
              }
            }
          } while (plVar16 != plVar2);
          goto code_r0x000180557b62;
        }
        goto code_r0x000180557b67;
      }
      FUN_1802807e0(&puStack_268,puStack_268[1]);
      puStack_268[1] = puStack_268;
      *puStack_268 = puStack_268;
      puStack_268[2] = puStack_268;
      uStack_260 = 0;
      func_0x000180bc5720(puStack_268);
      goto code_r0x000180557b8f;
    }
code_r0x000180557bb0:
    plVar18 = plVar18 + 1;
  } while( true );
code_r0x000180557680:
  lVar15 = memchr(&UNK_180e7aac4,(int)*pcVar20,1);
  if (lVar15 != 0) goto code_r0x0001805576ae;
  pcVar11 = (char *)func_0x0001800dad90(&uStack_1a8);
  if (pcVar20 == pcVar11) {
    iVar19 = -1;
    goto code_r0x0001805576c5;
  }
  pcVar20 = pcVar20 + -1;
  goto code_r0x000180557680;
code_r0x0001805576ae:
  iVar19 = (int)&uStack_1a8;
  if (0xf < uStack_190) {
    iVar19 = (int)CONCAT71(uStack_1a7,uStack_1a8);
  }
  iVar19 = (int)pcVar20 - iVar19;
code_r0x0001805576c5:
  uVar17 = (ulonglong)iVar19;
  FUN_1800d3650(&uStack_1a8,&uStack_128,0,uVar17);
  if (uVar17 < uStack_198) {
    puVar14 = &uStack_1a8;
    if (0xf < uStack_190) {
      puVar14 = (undefined1 *)CONCAT71(uStack_1a7,uStack_1a8);
    }
    pcVar20 = puVar14 + uStack_198;
    puVar14 = &uStack_1a8;
    if (0xf < uStack_190) {
      puVar14 = (undefined1 *)CONCAT71(uStack_1a7,uStack_1a8);
    }
    for (pcVar11 = puVar14 + uVar17; pcVar11 < pcVar20; pcVar11 = pcVar11 + 1) {
      lVar15 = memchr(&UNK_180e3fb14,(int)*pcVar11,1);
      if (lVar15 != 0) {
        puVar14 = &uStack_1a8;
        if (0xf < uStack_190) {
          puVar14 = (undefined1 *)CONCAT71(uStack_1a7,uStack_1a8);
        }
        lVar15 = (longlong)pcVar11 - (longlong)puVar14;
        goto code_r0x00018055774c;
      }
    }
  }
  lVar15 = -1;
code_r0x00018055774c:
  puVar12 = (undefined8 *)FUN_1800d3650(&uStack_1a8,&bStack_58,(longlong)(iVar19 + 1),lVar15);
  if (0xf < (ulonglong)puVar12[3]) {
    puVar12 = (undefined8 *)*puVar12;
  }
  uVar10 = atoi(puVar12);
  if (0xf < uStack_40) {
    uVar5 = CONCAT71(uStack_57,bStack_58);
    uVar17 = uVar5;
    if (0xfff < uStack_40 + 1) {
      if ((bStack_58 & 0x1f) != 0) {
        _invalid_parameter_noinfo_noreturn();
        pcVar4 = (code *)swi(3);
        (*pcVar4)();
        return;
      }
      uVar17 = *(ulonglong *)(uVar5 - 8);
      if (uVar5 <= uVar17) {
        _invalid_parameter_noinfo_noreturn();
        pcVar4 = (code *)swi(3);
        (*pcVar4)();
        return;
      }
      if (uVar5 - uVar17 < 8) {
        _invalid_parameter_noinfo_noreturn();
        pcVar4 = (code *)swi(3);
        (*pcVar4)();
        return;
      }
      if (0x27 < uVar5 - uVar17) {
        _invalid_parameter_noinfo_noreturn();
        pcVar4 = (code *)swi(3);
        (*pcVar4)();
        return;
      }
    }
    func_0x000180bc5720(uVar17);
  }
  uStack_40 = 0xf;
  uStack_48 = 0;
  bStack_58 = 0;
  uStack_170 = 0xf;
  uStack_178 = 0;
  uStack_188 = 0;
  FUN_180040680(&uStack_188,&UNK_180ef1128,0x2c);
  uStack_130 = 0xf;
  uStack_138 = 0;
  uStack_148 = 0;
  FUN_180040680(&uStack_148,&UNK_180eda938,7);
  uStack_150 = 0xf;
  uStack_158 = 0;
  uStack_168 = 0;
  FUN_180040680(&uStack_168,&UNK_180de47b9,0);
  uStack_f0 = 0xf;
  uStack_f8 = 0;
  auStack_108[0] = 0;
  FUN_180040550(auStack_108,lStack_248 + 0x10,0,0xffffffffffffffff);
  uVar17 = uStack_178;
  uStack_d0 = 0xf;
  uStack_d8 = 0;
  uStack_e8 = 0;
  if (uStack_178 == 0xffffffffffffffff) {
    __Xlength_error_std__YAXPEBD_Z(&UNK_180de5148);
    goto code_r0x000180557bef;
  }
  if (uStack_178 < 0x10) {
    if (uStack_178 == 0) {
      uStack_d8 = 0;
      uStack_e8 = 0;
      goto code_r0x000180557932;
    }
  }
  else {
    FUN_18003f980(&uStack_e8,uStack_178,0);
code_r0x000180557932:
    if (uVar17 == 0) goto code_r0x000180557993;
  }
  puVar14 = &uStack_188;
  if (0xf < uStack_170) {
    puVar14 = (undefined1 *)CONCAT71(uStack_187,uStack_188);
  }
  puVar21 = (undefined1 *)CONCAT71(uStack_e7,uStack_e8);
  puVar13 = &uStack_e8;
  if (0xf < uStack_d0) {
    puVar13 = puVar21;
  }
  if (uVar17 != 0) {
    func_0x000180bc6f8a(puVar13,puVar14,uVar17);
    puVar21 = (undefined1 *)CONCAT71(uStack_e7,uStack_e8);
  }
  uStack_d8 = uVar17;
  puVar14 = &uStack_e8;
  if (0xf < uStack_d0) {
    puVar14 = puVar21;
  }
  puVar14[uVar17] = 0;
code_r0x000180557993:
  uStack_b0 = 0xf;
  uStack_b8 = 0;
  auStack_c8[0] = 0;
  FUN_180040550(auStack_c8,&uStack_128,0,0xffffffffffffffff);
  uStack_88 = 0xf;
  uStack_90 = 0;
  auStack_a0[0] = 0;
  uStack_a8 = uVar10;
  FUN_180040550(auStack_a0,&uStack_148,0,0xffffffffffffffff);
  uStack_68 = 0xf;
  uStack_70 = 0;
  auStack_80[0] = 0;
  FUN_180040550(auStack_80,&uStack_168,0,0xffffffffffffffff);
  FUN_18049cca0(0x18136fd88,auStack_108);
  FUN_18049bf90(auStack_108);
  if (0xf < uStack_150) {
    FUN_1800409e0(&uStack_168,CONCAT71(uStack_167,uStack_168),uStack_150 + 1);
  }
  uStack_150 = 0xf;
  uStack_158 = 0;
  uStack_168 = 0;
  if (0xf < uStack_130) {
    FUN_1800409e0(&uStack_148,CONCAT71(uStack_147,uStack_148),uStack_130 + 1);
  }
  uStack_130 = 0xf;
  uStack_138 = 0;
  uStack_148 = 0;
  if (0xf < uStack_170) {
    FUN_1800409e0(&uStack_188,CONCAT71(uStack_187,uStack_188),uStack_170 + 1);
  }
  uStack_170 = 0xf;
  uStack_178 = 0;
  uStack_188 = 0;
  if (0xf < uStack_110) {
    FUN_1800409e0(&uStack_128,CONCAT71(uStack_127,uStack_128),uStack_110 + 1);
  }
  uStack_110 = 0xf;
  uStack_118 = 0;
  uStack_128 = 0;
code_r0x000180557b24:
  if (0xf < uStack_190) {
    FUN_1800409e0(&uStack_1a8,CONCAT71(uStack_1a7,uStack_1a8),uStack_190 + 1);
  }
  uStack_190 = 0xf;
  uStack_198 = 0;
  uStack_1a8 = 0;
code_r0x000180557b4e:
  FUN_18027e300(&uStack_1e0);
  FUN_18003f330(auStack_218);
code_r0x000180557b62:
  iVar19 = (int)uStack_250;
code_r0x000180557b67:
  FUN_180288090(&puStack_268,auStack_220,*puStack_268,puStack_268);
  func_0x000180bc5720(puStack_268);
code_r0x000180557b8f:
  uStack_250 = (ulonglong)(iVar19 - 1U);
  if (iVar19 - 1U == 0xffffffff) goto code_r0x000180557bb0;
  goto code_r0x000180557400;
}

