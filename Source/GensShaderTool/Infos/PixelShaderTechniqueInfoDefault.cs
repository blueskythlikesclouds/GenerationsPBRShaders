﻿namespace GensShaderTool.Infos
{
    public class PixelShaderTechniqueInfoDefault : PixelShaderTechniqueInfo
    {
        private static readonly VertexShaderPermutation[] sVertexShaderPermutations =
        {
            new VertexShaderPermutation( VertexShaderSubPermutations.All, "none", "Default_@@" )
        };

        public static PixelShaderTechniqueInfoDefault Instance { get; } =
            new PixelShaderTechniqueInfoDefault();

        public PixelShaderTechniqueInfoDefault() : base( "default", "", sVertexShaderPermutations )
        {
        }
    }
}