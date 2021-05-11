using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection.Emit;
using System.Reflection.Metadata;
using System.Text;
using Vortice.D3DCompiler;

namespace GensShaderTool
{
    public static class ShaderTranslator
    {
        private static readonly HashSet<string> sTextureSemantics =
            new HashSet<string>(StringComparer.OrdinalIgnoreCase)
            {
                "1D",
                "2D",
                "3D",
                "CUBE"
            };

        private static readonly Dictionary<string, string> sTextureSwizzleMap =
            new Dictionary<string, string>
            {
                { "1D", "x" },
                { "2D", "xy" },
                { "3D", "xyz" },
                { "CUBE", "xyz" }
            };

        private static void ProcessParameterMap(TextWriter writer, List<ShaderParameter> parameters,
            Dictionary<string, string> paramMap, char prefix, string type)
        {
            foreach (var param in parameters)
            {
                if (param.Size > 1)
                {
                    for (int i = 0; i < param.Size; i++)
                    {
                        string register = $"{prefix}{param.Index + i}";
                        string name = $"{param.Name}[{i}]";
                        paramMap[register] = name;
                    }
                }
                else
                {
                    string register = $"{prefix}{param.Index}";
                    paramMap[register] = param.Name;
                }

                writer.WriteLine("{0} {1}{2} : register({3}{4});", type, param.Name,
                    param.Size > 1 ? $"[{param.Size}]" : "", prefix, param.Index);
            }

            if (parameters.Count > 0)
                writer.WriteLine();
        }

        public static void Translate(byte[] bytes, TextWriter writer)
        {
            string[] lines;
            var shaderType = ShaderType.Pixel;

            unsafe
            {
                fixed (byte* ptr = bytes)
                {
                    Compiler.Disassemble(new IntPtr(ptr), bytes.Length, DisasmFlags.InstructionOnly, null, out var blob);
                    lines = blob.ConvertToString().Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
                }
            }
            
            var paramSet = ShaderParameterConverter.ReadConstantTable(new MemoryStream(bytes));
            var paramMap = new Dictionary<string, string>();

            var semantics = new List<(string, string)>();
            var constants = new Dictionary<string, (string, string, string, string)>();

            int i;
            for (i = 0; i < lines.Length; i++)
            {
                string line = lines[i];

                if (line.StartsWith("//"))
                    continue;

                if (line.StartsWith("vs_"))
                {
                    shaderType = ShaderType.Vertex;
                }

                else if (line.StartsWith("ps_"))
                {
                    shaderType = ShaderType.Pixel;
                }

                else if (line.StartsWith("def"))
                {
                    var split = line.Split(',');
                    constants.Add(split[0].Substring(split[0].IndexOf(' ') + 1),
                        (split[1].Trim(), split[2].Trim(), split[3].Trim(), split[4].Trim()));
                }

                else if (line.StartsWith("dcl"))
                {
                    var split = line.Split(' ', StringSplitOptions.RemoveEmptyEntries);
                    var split2 = split[0].Split('_');

                    semantics.Add((split2.Length > 1 ? split2[1].ToUpperInvariant() : "VPOS", split[1]));
                }
                else
                {
                    break;
                }
            }

            ProcessParameterMap(writer, paramSet.SingleParameters, paramMap, 'c', "float4");
            ProcessParameterMap(writer, paramSet.IntParameters, paramMap, 'i', "int");
            ProcessParameterMap(writer, paramSet.BoolParameters, paramMap, 'b', "bool");

            if (shaderType == ShaderType.Pixel)
            {
                foreach ((string type, string register) in semantics.Where(x => sTextureSemantics.Contains(x.Item1)))
                {
                    int index = int.Parse(register.AsSpan().Slice(1));
                    var param = paramSet.SamplerParameters.FirstOrDefault(x =>
                        index >= (x.Index & 0xF) && index < (x.Index & 0xF) + x.Size);

                    string name = register;

                    if (param != null)
                    {
                        name = param.Size > 1 ? $"{param.Name}{index - param.Index}" : param.Name;
                        paramMap[register] = name;
                    }

                    writer.WriteLine("sampler{0} {1} : register({2});", type, name, register);
                }

                writer.WriteLine();

                foreach (string texSemantic in sTextureSemantics)
                {
                    writer.WriteLine("float4 tex(sampler{0} s, float4 texCoord) {{ return tex{0}(s, texCoord.{1}); }}", texSemantic, sTextureSwizzleMap[texSemantic]);
                    writer.WriteLine("float4 texBias(sampler{0} s, float4 texCoord) {{ return tex{0}bias(s, texCoord); }}", texSemantic);
                    writer.WriteLine("float4 texLod(sampler{0} s, float4 texCoord) {{ return tex{0}lod(s, texCoord); }}", texSemantic);
                    writer.WriteLine("float4 texProj(sampler{0} s, float4 texCoord) {{ return tex{0}proj(s, texCoord); }}", texSemantic);
                }
            }

            writer.WriteLine("\nfloat4 normalize(float4 value) { return value / sqrt(dot(value.xyz, value.xyz)); }\n");

            string mainArguments = string.Join(" \n",
                    semantics.Where(x => !sTextureSemantics.Contains(x.Item1)).Select(x =>
                        $"\t{(x.Item2[0] == 'o' ? "out" : "in")} float4 {x.Item2.Split('.')[0]} : {x.Item1},"));

            writer.WriteLine(
                shaderType == ShaderType.Vertex
                    ? "void main(\n{0})\n{{"
                    : "void main(\n{0}\n\tout float4 oC0 : COLOR0,\n\tout float4 oC1 : COLOR1,\n\tout float4 oC2 : COLOR2,\n\tout float4 oC3 : COLOR3,\n\tout float oDepth : DEPTH)\n{{",
                shaderType == ShaderType.Vertex ? mainArguments.TrimEnd('\n', ',') : mainArguments);

            foreach (var constant in constants)
                writer.WriteLine("\tfloat4 {0} = float4{1};", constant.Key, string.Join(", ", constant.Value));

            if (constants.Count > 0)
                writer.WriteLine();

            for (int j = 0; j < 32; j++)
                writer.WriteLine("\tfloat4 r{0} = float4(0, 0, 0, 0);", j);

            writer.WriteLine("\tint4 a0 = int4(0, 0, 0, 0);");

            writer.WriteLine();

            int indent = 1;

            for (; i < lines.Length; i++)
            {
                string prettyLine = lines[i].Trim();
                if (prettyLine.StartsWith("//") || string.IsNullOrEmpty(prettyLine))
                    continue;

                var instruction = new Instruction(prettyLine);

                if (instruction.Arguments != null)
                {
                    foreach (var argument in instruction.Arguments)
                    {
                        if (paramMap.TryGetValue(argument.Token, out string paramName))
                            argument.Token = paramName;
                    }
                }

                string instrLine = instruction.ToString();

                // Stupid hack to make g_MtxPalette work
                instrLine = instrLine.Replace("][", " + ");

                if (instrLine.Contains('}')) indent--;

                for (int j = 0; j < indent; j++)
                    writer.Write("\t");

                writer.WriteLine("{0}", instrLine);

                if (instrLine.Contains('{')) indent++;
            }

            writer.WriteLine("}");
        }

        public static string Translate(byte[] bytes)
        {
            using var writer = new StringWriter();
            Translate(bytes, writer);
            return writer.ToString();
        }

        public static string Translate(string sourceFilePath)
        {
            return Translate(File.ReadAllBytes(sourceFilePath));
        }

        public static void Translate(byte[] bytes, string destinationFilePath)
        {
            using var writer = File.CreateText(destinationFilePath);
            Translate(bytes, writer);
        }        
        
        public static void Translate(string sourceFilePath, string destinationFilePath)
        {
            Translate(File.ReadAllBytes(sourceFilePath), destinationFilePath);
        }
    }

    public enum Swizzle
    {
        Empty = -1,
        X = 'x',
        Y = 'y',
        Z = 'z',
        W = 'w'
    }

    public class SwizzleSet
    {
        public static SwizzleSet Empty =>
            new SwizzleSet(Swizzle.Empty, Swizzle.Empty, Swizzle.Empty, Swizzle.Empty);      
        
        public static SwizzleSet XYZW =>
            new SwizzleSet(Swizzle.X, Swizzle.Y, Swizzle.Z, Swizzle.W);

        public Swizzle X;
        public Swizzle Y;
        public Swizzle Z;
        public Swizzle W;

        public Swizzle GetSwizzle(int index)
        {
            switch (index)
            {
                case 0: return X;
                case 1: return Y;
                case 2: return Z;
                case 3: return W;
            }

            return Swizzle.Empty;
        }

        public void SetSwizzle(int index, Swizzle swizzle)
        {
            switch (index)
            {
                case 0:
                    X = swizzle;
                    break;
                case 1:
                    Y = swizzle;
                    break;
                case 2:
                    Z = swizzle;
                    break;
                case 3:
                    W = swizzle;
                    break;
            }
        }

        public int Count
        {
            get
            {
                int count = X != Swizzle.Empty ? 1 : 0;
                count += Y != Swizzle.Empty ? 1 : 0;
                count += Z != Swizzle.Empty ? 1 : 0;
                count += W != Swizzle.Empty ? 1 : 0;

                return count;
            }
        }

        public void Convert(SwizzleSet dstSwizzleSet)
        {
            Expand(4);

            var newSwizzle = Empty;

            if (dstSwizzleSet.Count == 0)
                dstSwizzleSet = XYZW;

            for (int i = 0; i < 4; i++)
            {
                var swizzle = dstSwizzleSet.GetSwizzle(i);
                if (swizzle == Swizzle.Empty)
                    break;

                switch (swizzle)
                {
                    case Swizzle.X:
                        newSwizzle.SetSwizzle(i, GetSwizzle(0));
                        break;                    
                    
                    case Swizzle.Y:
                        newSwizzle.SetSwizzle(i, GetSwizzle(1));
                        break;                    
                    
                    case Swizzle.Z:
                        newSwizzle.SetSwizzle(i, GetSwizzle(2));
                        break;                   
                    
                    case Swizzle.W:
                        newSwizzle.SetSwizzle(i, GetSwizzle(3));
                        break;
                }
            }

            Replace(newSwizzle);
        }

        public void Shrink(int count)
        {
            int thisCount = Count;

            if (thisCount <= count)
                return;

            for (int i = count; i < 4; i++)
                SetSwizzle(i, Swizzle.Empty);
        }

        public void Expand(int count)
        {
            int thisCount = Count;

            if (thisCount >= count)
                return;

            if (thisCount == 0)
            {
                for (int i = 0; i < count; i++)
                    SetSwizzle(i, XYZW.GetSwizzle(i));
            }
            else
            {
                for (int i = thisCount; i < count; i++)
                    SetSwizzle(i, GetSwizzle(i - 1));
            }
        }

        public void Resize(int count)
        {
            int thisCount = Count;

            if (thisCount > count)
                Shrink(count);

            else if (thisCount < count)
                Expand(count);
        }

        public void Simplify()
        {
            if (X == Swizzle.X && Y == Swizzle.Y && Z == Swizzle.Z && W == Swizzle.W)
            {
                Replace(Empty);
            }

            else
            {
                bool allSame = true;

                for (int i = 1; i < Count; i++)
                {
                    if (GetSwizzle(i) == X)
                        continue;

                    allSame = false;
                    break;
                }

                if (allSame)
                {
                    Y = Swizzle.Empty;
                    Z = Swizzle.Empty;
                    W = Swizzle.Empty;
                }
            }
        }

        public void Replace(SwizzleSet swizzleSet)
        {
            X = swizzleSet.X;
            Y = swizzleSet.Y;
            Z = swizzleSet.Z;
            W = swizzleSet.W;
        }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder(4);

            for (int i = 0; i < 4; i++)
            {
                var swizzle = GetSwizzle(i);
                if (swizzle == Swizzle.Empty)
                    break;

                stringBuilder.Append((char)swizzle);
            }

            if (stringBuilder.Length > 0)
                stringBuilder.Insert(0, '.');

            return stringBuilder.ToString();
        }

        public SwizzleSet(Swizzle x, Swizzle y, Swizzle z, Swizzle w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }

        public SwizzleSet(ReadOnlySpan<char> argument)
        {
            X = Swizzle.Empty;
            Y = Swizzle.Empty;
            Z = Swizzle.Empty;
            W = Swizzle.Empty;

            for (int i = 0; i < argument.Length; i++)
                SetSwizzle(i, (Swizzle)argument[i]);
        }
    }

    public class Argument
    {
        public bool Not { get; set; }
        public bool Sign { get; set; }
        public string Token { get; set; }
        public SwizzleSet Swizzle { get; set; }
        public bool Abs { get; set; }
        public Argument Index { get; set; }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder();

            if (Not)
                stringBuilder.Append('!');

            if (Sign)
                stringBuilder.Append('-');

            if (Abs)
                stringBuilder.Append("abs(");

            stringBuilder.Append(Token);

            if (Index != null)
                stringBuilder.AppendFormat("[{0}]", Index);

            stringBuilder.Append(Swizzle);

            if (Abs)
                stringBuilder.Append(')');

            return stringBuilder.ToString();
        }

        public Argument(ReadOnlySpan<char> span)
        {
            span = span.Trim();

            if (span.StartsWith("!"))
            {
                span = span.Slice(1);
                Not = true;
            }

            if (span.StartsWith("-"))
            {
                span = span.Slice(1);
                Sign = true;
            }

            int absIndex = span.LastIndexOf("_abs");
            int periodIndex = span.LastIndexOf(".");
            int indexPos = span.IndexOf(']');

            var token = span;

            if (absIndex != -1 && absIndex > indexPos)
            {
                Abs = true;
                token = span.Slice(0, absIndex);
            }

            if (periodIndex != -1 && periodIndex > indexPos)
            {
                Swizzle = new SwizzleSet(span.Slice(periodIndex + 1));

                if (absIndex == -1)
                    token = span.Slice(0, periodIndex);
            }
            else
            {
                Swizzle = SwizzleSet.Empty;
            }

            indexPos = token.IndexOf('[');
            if (indexPos != -1)
            {
                Index = new Argument(token.Slice(indexPos + 1, token.IndexOf(']') - indexPos - 1));
                token = token.Slice(0, indexPos);
            }

            Token = token.ToString();
        }

        public Argument(string argument) : this(argument.AsSpan())
        {

        }
    }

    public enum ComparisonType
    {
        None,
        GreaterThan,
        LessThan,
        GreaterThanOrEqual,
        LessThanOrEqual,
        Equal,
        NotEqual
    }

    public class Instruction
    {
        public string OpCode { get; set; }
        public Argument[] Arguments { get; set; }
        public bool Saturate { get; set; }
        public ComparisonType ComparisonType { get; set; }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder();

            switch (OpCode)
            {
                case "abs":
                    Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);

                    stringBuilder.AppendFormat("{0} = abs({1});", Arguments[0], Arguments[1]);
                    break;

                case "add":
                    Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                    Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);

                    stringBuilder.AppendFormat("{0} = {1} + {2};", Arguments[0], Arguments[1], Arguments[2]);
                    break;

                case "cmp":
                    Arguments[1].Swizzle.Resize(1);
                    Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);
                    Arguments[3].Swizzle.Convert(Arguments[0].Swizzle);

                    stringBuilder.AppendFormat("{0} = {1} >= 0 ? {2} : {3};", Arguments[0], Arguments[1], Arguments[2], Arguments[3]);
                    break;

                case "dp2add":
                    Arguments[1].Swizzle.Resize(2);
                    Arguments[2].Swizzle.Resize(2);
                    Arguments[3].Swizzle.Resize(1);

                    stringBuilder.AppendFormat("{0} = dot({1}, {2}) + {3};", Arguments[0], Arguments[1], Arguments[2], Arguments[3]);
                    break;             
                
                case "dp3":
                    Arguments[1].Swizzle.Resize(3);
                    Arguments[2].Swizzle.Resize(3);

                    stringBuilder.AppendFormat("{0} = dot({1}, {2});", Arguments[0], Arguments[1], Arguments[2]);
                    break;       
                
                case "dp4":
                    Arguments[1].Swizzle.Resize(4);
                    Arguments[2].Swizzle.Resize(4);

                    stringBuilder.AppendFormat("{0} = dot({1}, {2});", Arguments[0], Arguments[1], Arguments[2]);
                    break;          
                
                case "dsx":
                    Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);

                    stringBuilder.AppendFormat("{0} = ddx({1});", Arguments[0], Arguments[1]);
                    break;        
                
                case "dsy":
                    Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);

                    stringBuilder.AppendFormat("{0} = ddy({1});", Arguments[0], Arguments[1]);
                    break;              
                
                case "exp":
                    Arguments[1].Swizzle.Resize(1);

                    stringBuilder.AppendFormat("{0} = exp2({1});", Arguments[0], Arguments[1]);
                    break;              
                
                case "frc":
                    Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);

                    stringBuilder.AppendFormat("{0} = frac({1});", Arguments[0], Arguments[1]);
                    break;

                case "log":
                    Arguments[1].Swizzle.Resize(1);

                    stringBuilder.AppendFormat("{0} = log2({1});", Arguments[0], Arguments[1]);
                    break;          
                
                case "lrp":
                    Arguments[1].Swizzle.Resize(1);
                    Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);
                    Arguments[3].Swizzle.Convert(Arguments[0].Swizzle);

                    stringBuilder.AppendFormat("{0} = lerp({1}, {2}, {3});", Arguments[0], Arguments[3], Arguments[2], Arguments[1]);
                    break;   

                case "mad":
                    Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                    Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);
                    Arguments[3].Swizzle.Convert(Arguments[0].Swizzle);

                    stringBuilder.AppendFormat("{0} = {1} * {2} + {3};", Arguments[0], Arguments[1], Arguments[2], Arguments[3]);
                    break;         
                
                case "mov":
                case "mova":
                    Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);

                    stringBuilder.AppendFormat("{0} = {1};", Arguments[0], Arguments[1]);
                    break;

                case "mul":
                    Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                    Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);

                    stringBuilder.AppendFormat("{0} = {1} * {2};", Arguments[0], Arguments[1], Arguments[2]);
                    break;             
                
                case "max":
                    Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                    Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);

                    stringBuilder.AppendFormat("{0} = max({1}, {2});", Arguments[0], Arguments[1], Arguments[2]);
                    break;              
                
                case "min":
                    Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                    Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);

                    stringBuilder.AppendFormat("{0} = min({1}, {2});", Arguments[0], Arguments[1], Arguments[2]);
                    break;      
                
                case "nrm":
                    Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);

                    stringBuilder.AppendFormat("{0} = normalize({1});", Arguments[0], Arguments[1]);
                    break;             
                
                case "pow":
                    Arguments[1].Swizzle.Resize(1);
                    Arguments[2].Swizzle.Resize(1);

                    stringBuilder.AppendFormat("{0} = pow(abs({1}), {2});", Arguments[0], Arguments[1], Arguments[2]);
                    break;               
                
                case "rcp":
                    Arguments[1].Swizzle.Resize(1);

                    stringBuilder.AppendFormat("{0} = rcp({1});", Arguments[0], Arguments[1]);
                    break;               
                
                case "rsq":
                    Arguments[1].Swizzle.Resize(1);

                    stringBuilder.AppendFormat("{0} = rsqrt({1});", Arguments[0], Arguments[1]);
                    break;      

                case "sub":
                    Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                    Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);

                    stringBuilder.AppendFormat("{0} = {1} - {2};", Arguments[0], Arguments[1], Arguments[2]);
                    break;

                case "texld":
                    stringBuilder.AppendFormat("{0} = tex({1}, {2});", Arguments[0], Arguments[2], Arguments[1]);
                    break;       
                
                case "texldb":
                    stringBuilder.AppendFormat("{0} = texBias({1}, {2});", Arguments[0], Arguments[2], Arguments[1]);
                    break;      
                
                case "texldl":
                    stringBuilder.AppendFormat("{0} = texLod({1}, {2});", Arguments[0], Arguments[2], Arguments[1]);
                    break;        
                
                case "texldp":
                    stringBuilder.AppendFormat("{0} = texProj({1}, {2});", Arguments[0], Arguments[2], Arguments[1]);
                    break;

                case "if":
                    stringBuilder.Append("if (");

                    switch (ComparisonType)
                    {
                        case ComparisonType.GreaterThan:
                            stringBuilder.AppendFormat("{0} > {1}", Arguments[0], Arguments[1]);
                            break;
                        case ComparisonType.LessThan:
                            stringBuilder.AppendFormat("{0} < {1}", Arguments[0], Arguments[1]);
                            break;
                        case ComparisonType.GreaterThanOrEqual:
                            stringBuilder.AppendFormat("{0} >= {1}", Arguments[0], Arguments[1]);
                            break;
                        case ComparisonType.LessThanOrEqual:
                            stringBuilder.AppendFormat("{0} <= {1}", Arguments[0], Arguments[1]);
                            break;
                        case ComparisonType.Equal:
                            stringBuilder.AppendFormat("{0} == {1}", Arguments[0], Arguments[1]);
                            break;
                        case ComparisonType.NotEqual:
                            stringBuilder.AppendFormat("{0} != {1}", Arguments[0], Arguments[1]);
                            break;
                        default:
                            stringBuilder.Append(Arguments[0]);
                            break;
                    }

                    stringBuilder.Append(") {");
                    break;

                case "else":
                    stringBuilder.Append("} else {");
                    break;

                case "endif":
                    stringBuilder.Append("}");
                    break;

                default:
                    stringBuilder.AppendFormat("// Unimplemented instruction: {0}", OpCode);

                    if (Saturate)
                        stringBuilder.Append("_sat");

                    if (Arguments == null) 
                        return stringBuilder.ToString();

                    for (var i = 0; i < Arguments.Length; i++)
                    {
                        if (i != 0)
                            stringBuilder.Append(',');

                        stringBuilder.AppendFormat(" {0}", Arguments[i]);
                    }

                    return stringBuilder.ToString();
            }

            if (Saturate)
                stringBuilder.AppendFormat(" {0} = saturate({0});", Arguments[0]);

            return stringBuilder.ToString();
        }

        public Instruction(string instruction)
        {
            var args = instruction.Split(',');

            var opArgsSplit = args[0].Split(' ');
            if (opArgsSplit.Length > 1)
                args[0] = opArgsSplit[1];
            else
                args = null;

            var opCodeSplit = opArgsSplit[0].Split('_');

            OpCode = opCodeSplit[0];

            foreach (string token in opCodeSplit)
            {
                switch (token)
                {
                    case "sat":
                        Saturate = true;
                        break;

                    case "gt":
                        ComparisonType = ComparisonType.GreaterThan;
                        break;                 
                    
                    case "lt":
                        ComparisonType = ComparisonType.LessThan;
                        break;                 
                    
                    case "ge":
                        ComparisonType = ComparisonType.GreaterThanOrEqual;
                        break;                   
                    
                    case "le":
                        ComparisonType = ComparisonType.LessThanOrEqual;
                        break;               
                    
                    case "eq":
                        ComparisonType = ComparisonType.Equal;
                        break;     
                    
                    case "ne":
                        ComparisonType = ComparisonType.NotEqual;
                        break;
                }
            }

            if (args != null)
                Arguments = args.Select(x => new Argument(x)).ToArray();
        }
    }
}