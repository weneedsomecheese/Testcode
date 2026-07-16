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

        // GOD MODE: CCharUser.OnHit -> return false
        ReplaceWithReturnBool(mod, "CCharUser", "OnHit", false, "God Mode");

        // ONE-HIT KILL: set fDmg = -999999 (NEGATIVE, because AddHP adds it to HP)
        SetFloatParam(mod, "CCharMob", "OnHit", 0, -999999f, "One-Hit Kill (CCharMob)");
        SetFloatParam(mod, "CCharBase", "OnHit", 0, -999999f, "One-Hit Kill (CCharBase)");

        // UNLIMITED AMMO
        ReplaceWithReturnVoid(mod, "CWeaponBase", "ConsumeBullet", "Unlimited Ammo (no consume)");
        ReplaceWithReturnBool(mod, "CWeaponBase", "get_IsBulletEmpty", false, "Unlimited Ammo (never empty)");

        // DAMAGE x10 (CalcWeaponDamage returns positive, but it gets negated before OnHit)
        MultiplyFloatReturn(mod, "CCharPlayer", "CalcWeaponDamage", 10f, "Damage x10");
        MultiplyFloatReturn(mod, "CCharBase", "CalcWeaponDamage", 10f, "Damage x10 (base)");

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

    static void Log(string label, string msg) => Console.WriteLine($"  [{label}] {msg}");

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

    static void SetFloatParam(ModuleDef mod, string typeName, string methodName, int pi, float val, string label)
    {
        var t = FindType(mod, typeName);
        if (t == null) { Log(label, $"SKIP: type '{typeName}' not found"); return; }
        var m = FindMethod(t, methodName);
        if (m == null) { Log(label, $"SKIP: method '{methodName}' not found"); return; }
        if (m.Body == null || m.Body.Instructions.Count == 0) { Log(label, "SKIP: empty body"); return; }

        var realParams = m.Parameters.Where(p => !p.IsHiddenThisParameter).ToList();
        if (pi >= realParams.Count) { Log(label, $"SKIP: param {pi} out of range"); return; }

        var param = realParams[pi];
        m.Body.Instructions.Insert(0, new Instruction(OpCodes.Ldc_R4, val));
        m.Body.Instructions.Insert(1, new Instruction(OpCodes.Starg_S, param));
        m.Body.UpdateInstructionOffsets();
        patches++;
        Log(label, $"OK: {typeName}.{methodName} param[{pi}] ({param.Name}) = {val}");
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
