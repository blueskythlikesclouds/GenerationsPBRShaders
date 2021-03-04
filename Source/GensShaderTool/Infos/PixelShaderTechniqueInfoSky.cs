namespace GensShaderTool.Infos
{
    public class PixelShaderTechniqueInfoSky : PixelShaderTechniqueInfo
    {
        private static readonly VertexShaderPermutation[] sVertexShaderPermutations =
        {
            new VertexShaderPermutation( VertexShaderSubPermutations.All, "none", "Sky_@@" )
        };

        public static PixelShaderTechniqueInfoSky Instance { get; } =
            new PixelShaderTechniqueInfoSky();

        public PixelShaderTechniqueInfoSky() : base( "default", "", sVertexShaderPermutations )
        {
        }
    }
}