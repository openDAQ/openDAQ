using Daq.Core.Types;


namespace openDaq.Net.Test;


public class CoreTypesStringTests : CoreTypesTestsBase
{
    //[SetUp]
    //public void Setup()
    //{
    //}

    //[TearDown]
    //public void TearDown()
    //{
    //}



    //GUID always checked in BaseObject constructor
    //[Test]
    //public void GuidTest()
    //{
    //    Type baseObjectInterfaceType = typeof(StringObject);
    //    Guid guid = baseObjectInterfaceType.GUID;

    //    Assert.Multiple(() =>
    //    {
    //        Assert.That(baseObjectInterfaceType.Name, Is.EqualTo(nameof(StringObject)));
    //        Assert.That(guid, Is.EqualTo(Guid.Parse("3d7f9d7d-8a70-5339-9038-b53f5fbf2442")));
    //    });
    //}

    [Test]
    public void CreateTest()
    {
        string expectedValue = "Hello";

        ErrorCode errorCode = default;
        StringObject? testObject = default;
        string? actualValue = default;
        bool? isEqual = default;

        Assert.DoesNotThrow(() => errorCode = CoreTypesFactory.CreateString(out testObject, expectedValue));
        Assert.DoesNotThrow(() => actualValue = testObject?.GetCharPtr());
        Assert.DoesNotThrow(() => isEqual = testObject?.Equals(expectedValue));

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
    public void CreateTest2()
    {
        string expectedValue = "Hello";

        StringObject? testObject = default;
        string? actualValue = default;
        bool? isEqual = default;

        Assert.DoesNotThrow(() => testObject = CoreTypesFactory.CreateString(expectedValue));
        Assert.DoesNotThrow(() => actualValue = testObject?.GetCharPtr());
        Assert.DoesNotThrow(() => isEqual = testObject?.Equals(expectedValue));

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
    public void CreateImplicitTest()
    {
        string expectedValue = "Hello";

        StringObject? testObject = default;
        string? actualValue = default;
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
    public void CreateExplicitTest()
    {
        string expectedValue = "Hello";

        StringObject? testObject = default;
        string? actualValue = default;
        bool isEqual = default;

        Assert.DoesNotThrow(() => testObject = (StringObject)expectedValue);
        Assert.DoesNotThrow(() => actualValue = (string)testObject);
        Assert.DoesNotThrow(() => isEqual = ((string)testObject == expectedValue));

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

    [TestCase("123")]
    [TestCase("1234")]
    [TestCase("12345")]
    [TestCase("123456")]
    public void GetCharPtrTest(string expectedValue)
    {
        StringObject testObject = expectedValue;

        string returnedValue = testObject.GetCharPtr();
        bool isEqual = testObject.Equals(expectedValue);

        Assert.Multiple(() =>
        {
            Assert.That(returnedValue, Is.EqualTo(expectedValue), "GetCharPtr() returned different value");
            Assert.That(isEqual, Is.True, "Equals() returned 'false'");
        });

        testObject.Dispose();
    }

    [TestCase("123")]
    [TestCase("1234")]
    [TestCase("12345")]
    [TestCase("123456")]
    public void GetStringImplicitTest(string expectedValue)
    {
        StringObject testObject = expectedValue;

        string returnedValue = testObject;
        bool isEqual = (testObject == expectedValue);

        Assert.Multiple(() =>
        {
            Assert.That(returnedValue, Is.EqualTo(expectedValue), "implicit cast returned different value");
            Assert.That(isEqual, Is.True, "'==' returned 'false'");
        });

        testObject.Dispose();
    }

    [TestCase("")]
    [TestCase("1")]
    [TestCase("12")]
    [TestCase("123")]
    [TestCase("1234")]
    [TestCase("12345")]
    [TestCase("123456")]
    public void GetStringExplicitTest(string expectedValue)
    {
        StringObject testObject = expectedValue;

        string returnedValue = (string)testObject;
        bool isEqual = (testObject == (StringObject)expectedValue);

        Assert.Multiple(() =>
        {
            Assert.That(returnedValue, Is.EqualTo(expectedValue), "explicit cast returned different value");
            Assert.That(isEqual, Is.True, "'==' returned 'false'");
        });

        testObject.Dispose();
    }

    [TestCase("123")]
    [TestCase("1234")]
    [TestCase("12345")]
    [TestCase("123456")]
    public void GetLengthTest(string expectedValue)
    {
        StringObject testObject = expectedValue;

        nuint length = testObject.GetLength();

        Assert.That(length, Is.EqualTo((nuint)expectedValue.Length));

        testObject.Dispose();
    }

    /*
        def test_create(self):
            daq.String('Hello')

        def test_ascii_string(self):
            s = daq.String(ascii_string)
            self.assertNotEqual(s, 'world')
            self.assertEqual(s, ascii_string)
            self.assertEqual(s.length, len(ascii_string))

        def test_unicode_string(self):
            s = daq.String(unicode_string)
            self.assertEqual(s, unicode_string)
            # length of UTF-8 strings doesn't seem to be correct
            #self.assertEqual(s.length, len(unicode_string))

        def test_hash(self):
            self.assertEqual(hash(daq.String(empty_string)), 0)
            self.assertNotEqual(hash(daq.String(ascii_string)), 0)

        def test_string_conversion(self):
            self.assertEqual(str(daq.String(empty_string)), empty_string)
            self.assertEqual(str(daq.String(ascii_string)), ascii_string)
            self.assertEqual(str(daq.String(unicode_string)), unicode_string)

        def test_core_type(self):
            self.assertEqual(daq.String(ascii_string).core_type, daq.CoreType.ctString)

        def test_can_cast_from(self):
            self.assertTrue(daq.IBaseObject.can_cast_from(daq.String('test')))

        def test_intConversion(self):
            self.assertEqual(int(daq.String('10')), 10)
    */
}
