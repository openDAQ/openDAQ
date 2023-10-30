using Daq.Core.Types;


namespace openDaq.Net.Test;


public class CoreTypesBoolTests : CoreTypesTestsBase
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
        bool expectedValue = true;

        ErrorCode errorCode = default;
        BoolObject? testObject = default;
        bool? actualValue = default;
        bool? isEqual = default;

        Assert.DoesNotThrow(() => errorCode = CoreTypesFactory.CreateBoolean(out testObject, expectedValue));
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
        bool expectedValue = true;

        BoolObject? testObject = default;
        bool? actualValue = default;
        bool? isEqual = default;

        Assert.DoesNotThrow(() => testObject = CoreTypesFactory.CreateBoolean(expectedValue));
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
        bool expectedValue = true;

        BoolObject? testObject = default;
        bool actualValue = default;
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
        bool expectedValue = true;

        BoolObject? testObject = default;
        bool actualValue = default;
        bool isEqual = default;

        Assert.DoesNotThrow(() => testObject = (BoolObject)expectedValue);
        Assert.DoesNotThrow(() => actualValue = (bool)testObject);
        Assert.DoesNotThrow(() => isEqual = ((bool)testObject == expectedValue));

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

    [TestCase(true)]
    [TestCase(false)]
    public void GetValueTest(bool expectedValue)
    {
        BoolObject testObject = expectedValue;

        bool returnedValue = testObject.GetValue();
        bool isEqual = testObject.EqualsValue(expectedValue);

        Assert.Multiple(() =>
        {
            Assert.That(returnedValue, Is.EqualTo(expectedValue), "GetValue() returned different value");
            Assert.That(isEqual, Is.True, "EqualsValue() returned 'false'");
        });

        testObject.Dispose();
    }

    [TestCase(true)]
    [TestCase(false)]
    public void GetValueImplicitTest(bool expectedValue)
    {
        BoolObject testObject = expectedValue;

        bool returnedValue = testObject;
        bool isEqual = (testObject == expectedValue);

        Assert.Multiple(() =>
        {
            Assert.That(returnedValue, Is.EqualTo(expectedValue), "implicit cast returned different value");
            Assert.That(isEqual, Is.True, "'==' returned 'false'");
        });

        testObject.Dispose();
    }

    [TestCase(true)]
    [TestCase(false)]
    public void GetValueExplicitTest(bool expectedValue)
    {
        BoolObject testObject = expectedValue;

        bool returnedValue = (bool)testObject;
        bool isEqual = (testObject == (BoolObject)expectedValue);

        Assert.Multiple(() =>
        {
            Assert.That(returnedValue, Is.EqualTo(expectedValue), "explicit cast returned different value");
            Assert.That(isEqual, Is.True, "'==' returned 'false'");
        });

        testObject.Dispose();
    }
}
