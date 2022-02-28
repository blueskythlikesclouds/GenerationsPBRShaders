namespace GensShaderTool.Compilers.Interop;

[StructLayout(LayoutKind.Sequential)]
public struct D3DShaderMacro
{
    public static readonly D3DShaderMacro Terminator;

    [MarshalAs(UnmanagedType.LPStr)]
    public string Name;    
    
    [MarshalAs(UnmanagedType.LPStr)]
    public string Definition;

    public D3DShaderMacro(string name, string definition)
    {
        Name = name;
        Definition = definition;
    }

    public D3DShaderMacro(string name) : this(name, null)
    {
    }

    public override string ToString()
    {
        return string.IsNullOrEmpty(Definition) ? Name : $"{Name}={Definition}";
    }
}