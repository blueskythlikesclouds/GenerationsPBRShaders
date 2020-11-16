namespace GensShaderTool
{
    public class SamplerInfo
    {
        public string Name { get; }
        public string Unit { get; }
        public string Suffix { get; }

        public SamplerInfo( string name, string unit, string suffix )
        {
            Name = name;
            Unit = unit;
            Suffix = suffix;
        }
    }
}