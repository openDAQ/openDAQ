using Daq.Core.Types;


namespace openDaq.Net.Test;


public class CoreTypesIntegerTests : CoreTypesTestsBase
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
        long expectedValue = 123;

        ErrorCode errorCode = default;
        IntegerObject? testObject = default;
        long? actualValue = default;
        bool? isEqual = default;

        Assert.DoesNotThrow(() => errorCode = CoreTypesFactory.CreateInteger(out testObject, expectedValue));
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
        long expectedValue = 123;

        IntegerObject? testObject = default;
        long? actualValue = default;
        bool? isEqual = default;

        Assert.DoesNotThrow(() => testObject = CoreTypesFactory.CreateInteger(expectedValue));
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
        long expectedValue = 123;

        IntegerObject? testObject = default;
        long actualValue = default;
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
        long expectedValue = 123;

        IntegerObject? testObject = default;
        long actualValue = default;
        bool isEqual = default;

        Assert.DoesNotThrow(() => testObject = (IntegerObject)expectedValue);
        Assert.DoesNotThrow(() => actualValue = (long)testObject);
        Assert.DoesNotThrow(() => isEqual = ((long)testObject == expectedValue));

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

    [TestCase(123)]
    [TestCase(1234)]
    [TestCase(12345)]
    [TestCase(123456)]
    public void GetValueTest(long expectedValue)
    {
        IntegerObject testObject = expectedValue;

        long returnedValue = testObject.GetValue();
        bool isEqual = testObject.EqualsValue(expectedValue);

        Assert.Multiple(() =>
        {
            Assert.That(returnedValue, Is.EqualTo(expectedValue), "GetValue() returned different value");
            Assert.That(isEqual, Is.True, "EqualsValue() returned 'false'");
        });

        testObject.Dispose();
    }

    [TestCase(123)]
    [TestCase(1234)]
    [TestCase(12345)]
    [TestCase(123456)]
    public void GetValueImplicitTest(long expectedValue)
    {
        IntegerObject testObject = expectedValue;

        long returnedValue = testObject;
        bool isEqual = (testObject == expectedValue);

        Assert.Multiple(() =>
        {
            Assert.That(returnedValue, Is.EqualTo(expectedValue), "implicit cast returned different value");
            Assert.That(isEqual, Is.True, "'==' returned 'false'");
        });

        testObject.Dispose();
    }

    [TestCase(123)]
    [TestCase(1234)]
    [TestCase(12345)]
    [TestCase(123456)]
    public void GetValueExplicitTest(long expectedValue)
    {
        IntegerObject testObject = expectedValue;

        long returnedValue = (long)testObject;
        bool isEqual = (testObject == (IntegerObject)expectedValue);

        Assert.Multiple(() =>
        {
            Assert.That(returnedValue, Is.EqualTo(expectedValue), "explicit cast returned different value");
            Assert.That(isEqual, Is.True, "'==' returned 'false'");
        });

        testObject.Dispose();
    }
}
