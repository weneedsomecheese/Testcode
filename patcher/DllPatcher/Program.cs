using dnlib.DotNet;
using dnlib.DotNet.Emit;

class Program
{
    static int patches = 0;

    static void Main(string[] args)
    {
        if (args.Length < 1)
        {
            Console.WriteLine("Usage: DllPatcher <input.dll> [output.dll]");
            return;
        }

        string input = args[0];
        string output = args.Length > 1 ? args[1]
            : Path.Combine(Path.GetDirectoryName(input) ?? ".", "Assembly-CSharp-patched.dll");

        Console.WriteLine($"Loading {input}...");
        var mod = ModuleDefMD.Load(input);
        Console.WriteLine($"Module: {mod.Name}, Types: {mod.Types.Count}");

        // GOD MODE: CCharUser.OnHit -> return false (you take no damage)
        ReplaceWithReturnBool(mod, "CCharUser", "OnHit", false, "God Mode");

        // ONE-HIT KILL: set mob HP to 0 directly when OnHit is called
        PatchOneHitKill(mod, "One-Hit Kill");

        // UNLIMITED AMMO
        ReplaceWithReturnVoid(mod, "CWeaponBase", "ConsumeBullet", "Unlimited Ammo (no consume)");
        ReplaceWithReturnBool(mod, "CWeaponBase", "get_IsBulletEmpty", false, "Unlimited Ammo (never empty)");

        // DAMAGE x10000
        MultiplyFloatReturn(mod, "CCharPlayer", "CalcWeaponDamage", 10000f, "Damage x10000");
        // Also patch base class in case CCharMob uses it directly
        MultiplyFloatReturn(mod, "CCharBase", "CalcWeaponDamage", 10000f, "Damage x10000 (base)");

        // GOLD x10
        MultiplyIntParam(mod, "iDataCenter", "AddGold", 0, 10, "Gold x10 (iDataCenter)");
        MultiplyIntParam(mod, "CCharUser", "AddGold", 0, 10, "Gold x10 (CCharUser)");

        // CRYSTAL x10
        MultiplyIntParam(mod, "iDataCenter", "AddCrystal", 0, 10, "Crystal x10 (iDataCenter)");

        // EXP x10
        MultiplyIntParam(mod, "CCharUser", "AddExp", 0, 10, "Exp x10 (CCharUser)");

        Console.WriteLine($"\n=== {patches} patches applied ===");
        mod.Write(output);
        Console.WriteLine($"Saved: {output}");
    }

    static TypeDef? FindType(ModuleDef mod, string name)
    {
        foreach (var t in mod.Types)
        {
            if (t.Name == name) return t;
            foreach (var n in t.NestedTypes)
                if (n.Name == name) return n;
        }
        return null;
    }

    static MethodDef? FindMethod(TypeDef type, string name)
    {
        foreach (var m in type.Methods)
            if (m.Name == name) return m;
        return null;
    }

    static FieldDef? FindField(TypeDef type, string name)
    {
        foreach (var f in type.Fields)
            if (f.Name == name) return f;
        return null;
    }

    static void Log(string label, string msg) => Console.WriteLine($"  [{label}] {msg}");

    static void PatchOneHitKill(ModuleDef mod, string label)
    {
        var mobType = FindType(mod, "CCharMob");
        if (mobType == null) { Log(label, "SKIP: CCharMob not found"); return; }
        var onHit = FindMethod(mobType, "OnHit");
        if (onHit == null) { Log(label, "SKIP: OnHit not found on CCharMob"); return; }
        if (onHit.Body == null || onHit.Body.Instructions.Count == 0) { Log(label, "SKIP: empty body"); return; }

        // Find m_fHP field on CCharBase (parent class)
        var baseType = FindType(mod, "CCharBase");
        if (baseType == null) { Log(label, "SKIP: CCharBase not found"); return; }
        var hpField = FindField(baseType, "m_fHP");
        if (hpField == null) { Log(label, "SKIP: m_fHP field not found"); return; }

        var instrs = onHit.Body.Instructions;

        // Prepend: this.m_fHP = 0f;
        // IL: ldarg.0; ldc.r4 0.0; stfld CCharBase::m_fHP
        instrs.Insert(0, new Instruction(OpCodes.Ldarg_0));
        instrs.Insert(1, new Instruction(OpCodes.Ldc_R4, 0f));
        instrs.Insert(2, new Instruction(OpCodes.Stfld, hpField));

        // Also set fDmg = 999999 as backup
        var realParams = onHit.Parameters.Where(p => !p.IsHiddenThisParameter).ToList();
        if (realParams.Count > 0)
        {
            instrs.Insert(3, new Instruction(OpCodes.Ldc_R4, 999999f));
            instrs.Insert(4, new Instruction(OpCodes.Starg_S, realParams[0]));
        }

        onHit.Body.UpdateInstructionOffsets();
        patches++;
        Log(label, $"OK: CCharMob.OnHit -> this.m_fHP = 0 + fDmg = 999999");

        // Also patch CCharBase.OnHit to set HP to 0 (in case it's called directly)
        if (baseType != null)
        {
            var baseOnHit = FindMethod(baseType, "OnHit");
            if (baseOnHit?.Body != null && baseOnHit.Body.Instructions.Count > 0)
            {
                var bi = baseOnHit.Body.Instructions;
                bi.Insert(0, new Instruction(OpCodes.Ldarg_0));
                bi.Insert(1, new Instruction(OpCodes.Ldc_R4, 0f));
                bi.Insert(2, new Instruction(OpCodes.Stfld, hpField));

                var baseParams = baseOnHit.Parameters.Where(p => !p.IsHiddenThisParameter).ToList();
                if (baseParams.Count > 0)
                {
                    bi.Insert(3, new Instruction(OpCodes.Ldc_R4, 999999f));
                    bi.Insert(4, new Instruction(OpCodes.Starg_S, baseParams[0]));
                }
                baseOnHit.Body.UpdateInstructionOffsets();
                patches++;
                Log(label, $"OK: CCharBase.OnHit -> this.m_fHP = 0 + fDmg = 999999");
            }
        }
    }

    static void ReplaceWithReturnBool(ModuleDef mod, string typeName, string methodName, bool value, string label)
    {
        var t = FindType(mod, typeName);
        if (t == null) { Log(label, $"SKIP: type '{typeName}' not found"); return; }
        var m = FindMethod(t, methodName);
        if (m == null) { Log(label, $"SKIP: method '{methodName}' not found"); return; }
        if (m.Body == null) { Log(label, "SKIP: no body"); return; }

        m.Body.Instructions.Clear();
        m.Body.ExceptionHandlers.Clear();
        m.Body.Instructions.Add(new Instruction(value ? OpCodes.Ldc_I4_1 : OpCodes.Ldc_I4_0));
        m.Body.Instructions.Add(new Instruction(OpCodes.Ret));
        m.Body.UpdateInstructionOffsets();
        patches++;
        Log(label, $"OK: {typeName}.{methodName} -> return {value}");
    }

    static void ReplaceWithReturnVoid(ModuleDef mod, string typeName, string methodName, string label)
    {
        var t = FindType(mod, typeName);
        if (t == null) { Log(label, $"SKIP: type '{typeName}' not found"); return; }
        var m = FindMethod(t, methodName);
        if (m == null) { Log(label, $"SKIP: method '{methodName}' not found"); return; }
        if (m.Body == null) { Log(label, "SKIP: no body"); return; }

        m.Body.Instructions.Clear();
        m.Body.ExceptionHandlers.Clear();
        m.Body.Instructions.Add(new Instruction(OpCodes.Ret));
        m.Body.UpdateInstructionOffsets();
        patches++;
        Log(label, $"OK: {typeName}.{methodName} -> return void");
    }

    static void MultiplyIntParam(ModuleDef mod, string typeName, string methodName, int pi, int mul, string label)
    {
        var t = FindType(mod, typeName);
        if (t == null) { Log(label, $"SKIP: type '{typeName}' not found"); return; }
        var m = FindMethod(t, methodName);
        if (m == null) { Log(label, $"SKIP: method '{methodName}' not found"); return; }
        if (m.Body == null || m.Body.Instructions.Count == 0) { Log(label, "SKIP: empty body"); return; }

        var realParams = m.Parameters.Where(p => !p.IsHiddenThisParameter).ToList();
        if (pi >= realParams.Count) { Log(label, $"SKIP: param {pi} out of range"); return; }

        var param = realParams[pi];
        var instrs = m.Body.Instructions;
        instrs.Insert(0, new Instruction(OpCodes.Ldarg, param));
        instrs.Insert(1, new Instruction(OpCodes.Ldc_I4, mul));
        instrs.Insert(2, new Instruction(OpCodes.Mul));
        instrs.Insert(3, new Instruction(OpCodes.Starg, param));
        m.Body.UpdateInstructionOffsets();
        patches++;
        Log(label, $"OK: {typeName}.{methodName} param[{pi}] ({param.Name}) *= {mul}");
    }

    static void MultiplyFloatReturn(ModuleDef mod, string typeName, string methodName, float mul, string label)
    {
        var t = FindType(mod, typeName);
        if (t == null) { Log(label, $"SKIP: type '{typeName}' not found"); return; }
        var m = FindMethod(t, methodName);
        if (m == null) { Log(label, $"SKIP: method '{methodName}' not found"); return; }
        if (m.Body == null) { Log(label, "SKIP: no body"); return; }

        var instrs = m.Body.Instructions;
        var rets = instrs.Where(i => i.OpCode == OpCodes.Ret).ToList();
        if (rets.Count == 0) { Log(label, "SKIP: no ret"); return; }

        foreach (var ret in rets)
        {
            int idx = instrs.IndexOf(ret);
            var ldc = new Instruction(OpCodes.Ldc_R4, mul);
            var mulOp = new Instruction(OpCodes.Mul);
            instrs.Insert(idx, ldc);
            instrs.Insert(idx + 1, mulOp);

            foreach (var insn in instrs)
            {
                if (insn.Operand is Instruction target && target == ret)
                    insn.Operand = ldc;
                else if (insn.Operand is Instruction[] targets)
                    for (int i = 0; i < targets.Length; i++)
                        if (targets[i] == ret) targets[i] = ldc;
            }
            foreach (var eh in m.Body.ExceptionHandlers)
            {
                if (eh.TryStart == ret) eh.TryStart = ldc;
                if (eh.TryEnd == ret) eh.TryEnd = ldc;
                if (eh.HandlerStart == ret) eh.HandlerStart = ldc;
                if (eh.HandlerEnd == ret) eh.HandlerEnd = ldc;
                if (eh.FilterStart == ret) eh.FilterStart = ldc;
            }
        }

        m.Body.UpdateInstructionOffsets();
        patches++;
        Log(label, $"OK: {typeName}.{methodName} return *= {mul} ({rets.Count} rets)");
    }
}
