namespace GensShaderTool.Infos
{
    public class PixelShaderTechniqueInfoDetailBlend : PixelShaderTechniqueInfo
    {
        private static readonly VertexShaderPermutation[] sVertexShaderPermutations =
        {
            new VertexShaderPermutation( VertexShaderSubPermutations.All, "none", "DetailBlend" )
        };

        public static PixelShaderTechniqueInfoDetailBlend Instance { get; } =
            new PixelShaderTechniqueInfoDetailBlend();

        public PixelShaderTechniqueInfoDetailBlend() : base( "default", "", sVertexShaderPermutations )
        {
        }
    }
}