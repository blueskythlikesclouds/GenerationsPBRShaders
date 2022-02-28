namespace GensShaderTool.Compilers.Interop;

public class D3DCompiler
{
    [DllImport("d3dcompiler_47.dll", CallingConvention = CallingConvention.StdCall, PreserveSig = true, EntryPoint = "D3DCompile")]
    public static extern int Compile(
        [MarshalAs(UnmanagedType.LPStr), In] string pSrcData,
        [In] int SrcDataSize,
        [MarshalAs(UnmanagedType.LPStr), In, Optional] string pSourceName,
        [In, Optional] D3DShaderMacro[] pDefines,
        [In, Optional] IntPtr pInclude,
        [MarshalAs(UnmanagedType.LPStr), In, Optional] string pEntryPoint,
        [MarshalAs(UnmanagedType.LPStr), In] string pTarget,
        [In] uint Flags1,
        [In] uint Flags2,
        [Out] out ID3DBlob ppCode,
        [Out] out ID3DBlob ppErrorMsgs);
}