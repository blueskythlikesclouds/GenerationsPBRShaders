namespace GensShaderTool.Infos
{
    public class PixelShaderTechniqueInfoEye2 : PixelShaderTechniqueInfo
    {
        private static readonly VertexShaderPermutation[] sVertexShaderPermutations =
        {
            new VertexShaderPermutation( VertexShaderSubPermutations.All, "none", "Eye2" )
        };

        public static PixelShaderTechniqueInfoEye2 Instance { get; } =
            new PixelShaderTechniqueInfoEye2();

        public PixelShaderTechniqueInfoEye2() : base( "default", "", sVertexShaderPermutations )
        {
        }
    }
}