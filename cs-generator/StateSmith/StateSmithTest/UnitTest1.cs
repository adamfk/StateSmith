using StateSmith;
using System;
using Xunit;

namespace StateSmithTest
{
    public class UnitTest1
    {
        [Fact]
        public void Test1()
        {
            YedInputParser yedInputParser = new YedInputParser();
            yedInputParser.ReadFile();
        }
    }
}
