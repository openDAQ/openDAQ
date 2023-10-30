using Daq.Core.Types;


namespace openDaq.Net.Test;


public class CoreTypesFloatTests : CoreTypesTestsBase
{
    //[SetUp]
    //public void Setup()
    //{
    //}

    //[TearDown]
    //public void TearDown()
    //{
    //}



    [Test]
    public void CreateReturnErrorCodeTest()
    {
        double expectedValue = 123d;

        ErrorCode errorCode = default;
        FloatObject? testObject = default;
        double? actualValue = default;
        bool? isEqual = default;

        Assert.DoesNotThrow(() => errorCode = CoreTypesFactory.CreateFloat(out testObject, expectedValue));
        Assert.DoesNotThrow(() => actualValue = testObject?.GetValue());
        Assert.DoesNotThrow(() => isEqual = testObject?.EqualsValue(expectedValue));

        Assert.Multiple(() =>
        {
            Assert.That(errorCode, Is.EqualTo(ErrorCode.OPENDAQ_SUCCESS));
            Assert.That(testObject, Is.Not.Null);
            Assert.That(testObject?.IsDisposed, Is.False);
            Assert.That(actualValue, Is.EqualTo(expectedValue));
            Assert.That(isEqual, Is.True);
        });

        testObject!.Dispose();
        Assert.That(testObject.IsDisposed, Is.True);
    }

    [Test]
    public void CreateReturnObjectTest()
    {
        double expectedValue = 123d;

        FloatObject? testObject = default;
        double? actualValue = default;
        bool? isEqual = default;

        Assert.DoesNotThrow(() => testObject = CoreTypesFactory.CreateFloat(expectedValue));
        Assert.DoesNotThrow(() => actualValue = testObject?.GetValue());
        Assert.DoesNotThrow(() => isEqual = testObject?.EqualsValue(expectedValue));

        Assert.Multiple(() =>
        {
            Assert.That(testObject, Is.Not.Null);
            Assert.That(testObject?.IsDisposed, Is.False);
            Assert.That(actualValue, Is.EqualTo(expectedValue));
            Assert.That(isEqual, Is.True);
        });

        testObject!.Dispose();
        Assert.That(testObject.IsDisposed, Is.True);
    }

    [Test]
    public void CreateTestImplicit()
    {
        double expectedValue = 123d;

        FloatObject? testObject = default;
        double actualValue = default;
        bool isEqual = default;

        Assert.DoesNotThrow(() => testObject = expectedValue);
        Assert.DoesNotThrow(() => actualValue = testObject);
        Assert.DoesNotThrow(() => isEqual = (testObject == expectedValue));

        Assert.Multiple(() =>
        {
            Assert.That(testObject, Is.Not.Null);
            Assert.That(testObject?.IsDisposed, Is.False);
            Assert.That(actualValue, Is.EqualTo(expectedValue));
            Assert.That(isEqual, Is.True);
        });

        testObject!.Dispose();
        Assert.That(testObject.IsDisposed, Is.True);
    }

    [Test]
    public void CreateTestExplicit()
    {
        double expectedValue = 123d;

        FloatObject? testObject = default;
        double actualValue = default;
        bool isEqual = default;

        Assert.DoesNotThrow(() => testObject = (FloatObject)expectedValue);
        Assert.DoesNotThrow(() => actualValue = (double)testObject);
        Assert.DoesNotThrow(() => isEqual = ((double)testObject == expectedValue));

        Assert.Multiple(() =>
        {
            Assert.That(testObject, Is.Not.Null);
            Assert.That(testObject?.IsDisposed, Is.False);
            Assert.That(actualValue, Is.EqualTo(expectedValue));
            Assert.That(isEqual, Is.True);
        });

        testObject!.Dispose();
        Assert.That(testObject.IsDisposed, Is.True);
    }

    [TestCase(123d)]
    [TestCase(1234d)]
    [TestCase(12345d)]
    [TestCase(123456d)]
    public void GetValueTest(double expectedValue)
    {
        FloatObject testObject = expectedValue;

        double returnedValue = testObject.GetValue();
        bool isEqual = testObject.EqualsValue(expectedValue);

        Assert.Multiple(() =>
        {
            Assert.That(returnedValue, Is.EqualTo(expectedValue), "GetValue() returned different value");
            Assert.That(isEqual, Is.True, "EqualsValue() returned 'false'");
        });

        testObject.Dispose();
    }

    [TestCase(123d)]
    [TestCase(1234d)]
    [TestCase(12345d)]
    [TestCase(123456d)]
    public void GetValueImplicitTest(double expectedValue)
    {
        FloatObject testObject = expectedValue;

        double returnedValue = testObject;
        bool isEqual = (testObject == expectedValue);

        Assert.Multiple(() =>
        {
            Assert.That(returnedValue, Is.EqualTo(expectedValue), "implicit cast returned different value");
            Assert.That(isEqual, Is.True, "'==' returned 'false'");
        });

        testObject.Dispose();
    }

    [TestCase(123d)]
    [TestCase(1234d)]
    [TestCase(12345d)]
    [TestCase(123456d)]
    public void GetValueExplicitTest(double expectedValue)
    {
        FloatObject testObject = expectedValue;

        double returnedValue = (double)testObject;
        bool isEqual = (testObject == (FloatObject)expectedValue);

        Assert.Multiple(() =>
        {
            Assert.That(returnedValue, Is.EqualTo(expectedValue), "explicit cast returned different value");
            Assert.That(isEqual, Is.True, "'==' returned 'false'");
        });

        testObject.Dispose();
    }
}
