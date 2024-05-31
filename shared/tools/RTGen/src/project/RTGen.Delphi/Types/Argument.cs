using RTGen.Interfaces;

namespace RTGen.Delphi.Types
{
    public class Argument : IArgument
    {
        public Argument()
        {
            IsConst = false;
            IsOutPointer = false;
            ArrayInfo = null;
            DefaultValue = null;
            IsPolymorphic = false;
            IsStealRef = false;
        }

        public ITypeName Type { get; set; }

        public string Name { get; set; }

        public bool IsOutParam { get; set; }

        public bool IsOutPointer { get; }

        public bool IsConst { get; }

        public IArray ArrayInfo { get; }

        public string DefaultValue { get; }

        public bool IsPolymorphic { get; set; }

        public bool IsStealRef { get; set; }

        public IArgument Clone()
        {
            return new Argument
            {
                Name = this.Name,
                IsOutParam = this.IsOutParam,
                Type = this.Type.Clone()
            };
        }

        public bool Equals(IArgument other)
        {
            if (other == null)
            {
                return false;
            }

            return IsConst == other.IsConst &&
                   Type.Equals(other.Type);

        }

        /// <summary>Human representation.</summary>
        public override string ToString()
        {
            return $"{(IsOutParam ? "out " : "")}{(Type != null ? Type.Name + " " : "")}{Name}";
        }
    }
}
