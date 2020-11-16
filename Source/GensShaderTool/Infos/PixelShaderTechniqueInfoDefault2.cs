namespace GensShaderTool.Infos
{
    public class PixelShaderTechniqueInfoDefault2 : PixelShaderTechniqueInfo
    {
        private static readonly VertexShaderPermutation[] sVertexShaderPermutations =
        {
            new VertexShaderPermutation( 3, "none", "Default2" )
        };

        public static PixelShaderTechniqueInfoDefault2 Instance { get; } =
            new PixelShaderTechniqueInfoDefault2();

        public PixelShaderTechniqueInfoDefault2() : base( "default", "", sVertexShaderPermutations )
        {
        }
    }
}