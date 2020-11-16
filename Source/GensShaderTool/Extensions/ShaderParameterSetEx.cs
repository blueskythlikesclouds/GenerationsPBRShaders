namespace GensShaderTool.Extensions
{
    public static class ShaderParameterSetEx
    {
        public static bool IsEmpty( this ShaderParameterSet shaderParameterSet )
        {
            return shaderParameterSet.SingleParameters.Count == 0 && shaderParameterSet.BoolParameters.Count == 0 &&
                   shaderParameterSet.IntParameters.Count == 0 && shaderParameterSet.SamplerParameters.Count == 0;
        }
    }
}