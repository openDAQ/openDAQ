using Daq.Core.Types;


namespace openDaq.Net.Test;


public class CoreTypesBaseObjectTests : CoreTypesTestsBase
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
    public void GuidTest()
    {
        Type testObjectInterfaceType = typeof(BaseObject);
        Guid guid = testObjectInterfaceType.GUID;

        Assert.Multiple(() =>
        {
            Assert.That(testObjectInterfaceType.Name, Is.EqualTo(nameof(BaseObject)));
            Assert.That(guid, Is.EqualTo(Guid.Parse("9c911f6d-1664-5aa2-97bd-90fe3143e881")));
        });
    }

    [Test]
    public void CreateTest()
    {
        bool isTrackingObjects = CoreTypes.IsTrackingObjects(); //returns if the SDK supports tracking generally (always true)

        ErrorCode errorCode = CoreTypesFactory.CreateBaseObject(out BaseObject testObject);

        Assert.Multiple(() =>
        {
            Assert.That(errorCode, Is.EqualTo(ErrorCode.OPENDAQ_SUCCESS));
            Assert.That(testObject, Is.Not.Null);
            Assert.That(testObject.IsDisposed, Is.False);
            if (isTrackingObjects) Assert.That(CoreTypes.GetTrackedObjectCount(), Is.EqualTo(1));
        });

        if (!testObject.IsDisposed) //not necessarily needed
            testObject.Dispose();

        Assert.Multiple(() =>
        {
            Assert.That(testObject.IsDisposed, Is.True);
            if (isTrackingObjects) Assert.That(CoreTypes.GetTrackedObjectCount(), Is.EqualTo(0));
        });
    }

    [Test]
    public void QueryInterfaceTest()
    {
        bool isTrackingObjects = CoreTypes.IsTrackingObjects(); //returns if the SDK supports tracking generally (always true)

        ErrorCode errorCode = CoreTypesFactory.CreateBaseObject(out BaseObject testObject);
        Assert.Multiple(() =>
        {
            Assert.That(errorCode, Is.EqualTo(ErrorCode.OPENDAQ_SUCCESS));
            if (isTrackingObjects) Assert.That(CoreTypes.GetTrackedObjectCount(), Is.EqualTo(1));
        });

        BaseObject queriedObject = testObject.QueryInterface<BaseObject>();

        Assert.Multiple(() =>
        {
            Assert.That(queriedObject, Is.Not.Null);
            if (isTrackingObjects) Assert.That(CoreTypes.GetTrackedObjectCount(), Is.EqualTo(1));
        });

        Assert.That(testObject.IsDisposed, Is.Not.True);
        testObject.Dispose();

        if (isTrackingObjects) Assert.That(CoreTypes.GetTrackedObjectCount(), Is.EqualTo(1)); //because QueryInterface() increments refCount

        Assert.That(queriedObject.IsDisposed, Is.Not.True);
        queriedObject.Dispose();

        if (isTrackingObjects) Assert.That(CoreTypes.GetTrackedObjectCount(), Is.EqualTo(0));
    }

    [Test]
    public void BorrowInterfaceTest()
    {
        bool isTrackingObjects = CoreTypes.IsTrackingObjects(); //returns if the SDK supports tracking generally (always true)

        ErrorCode errorCode = CoreTypesFactory.CreateBaseObject(out BaseObject testObject);
        Assert.Multiple(() =>
        {
            Assert.That(errorCode, Is.EqualTo(ErrorCode.OPENDAQ_SUCCESS));
            if (isTrackingObjects) Assert.That(CoreTypes.GetTrackedObjectCount(), Is.EqualTo(1));
        });

        //should just return the testObject since we don't "change" the type
        BaseObject queriedObject = testObject.BorrowInterface<BaseObject>();

        Assert.Multiple(() =>
        {
            Assert.That(queriedObject, Is.Not.Null);
            Assert.That(queriedObject, Is.SameAs(testObject));
            if (isTrackingObjects) Assert.That(CoreTypes.GetTrackedObjectCount(), Is.EqualTo(1));
        });

        Assert.That(testObject.IsDisposed, Is.Not.True);
        testObject.Dispose(); //also disposes of queriedObject since both point to the same object

        if (isTrackingObjects) Assert.That(CoreTypes.GetTrackedObjectCount(), Is.EqualTo(0)); //because BorrowInterface() doesn't increment refCount

        Assert.That(queriedObject.IsDisposed, Is.True);
        queriedObject.Dispose();

        if (isTrackingObjects) Assert.That(CoreTypes.GetTrackedObjectCount(), Is.EqualTo(0));
    }

    [Test]
    public void CastBoolObjectTest()
    {
        bool expectedValue = true;

        //get a BoolObject from bool as BaseObject
        BaseObject testObject = expectedValue;

        Assert.Multiple(() =>
        {
            Assert.That(testObject, Is.Not.Null);
            Assert.That(testObject.IsDisposed, Is.False);
            Assert.That(testObject, Is.TypeOf<BoolObject>());
        });

        //get the bool value from a 'BoolObject as BaseObject'
        bool value = testObject;

        Assert.Multiple(() =>
        {
            Assert.That(value, Is.EqualTo(expectedValue));
        });

        testObject.Dispose();
    }

    [Test]
    public void CastIntegerObjectTest()
    {
        long expectedValue = 1234;

        //get a IntegerObject from long as BaseObject
        BaseObject testObject = expectedValue;

        Assert.Multiple(() =>
        {
            Assert.That(testObject, Is.Not.Null);
            Assert.That(testObject.IsDisposed, Is.False);
            Assert.That(testObject, Is.TypeOf<IntegerObject>());
        });

        //get the long value from an 'IntegerObject as BaseObject'
        long value = testObject;

        Assert.Multiple(() =>
        {
            Assert.That(value, Is.EqualTo(expectedValue));
        });

        testObject.Dispose();
    }

    [Test]
    public void CastFloatObjectTest()
    {
        double expectedValue = 123d;

        //get a FloatObject from double as BaseObject
        BaseObject testObject = expectedValue;

        Assert.Multiple(() =>
        {
            Assert.That(testObject, Is.Not.Null);
            Assert.That(testObject.IsDisposed, Is.False);
            Assert.That(testObject, Is.TypeOf<FloatObject>());
        });

        //get the double value from a 'FloatObject as BaseObject'
        double value = testObject;

        Assert.Multiple(() =>
        {
            Assert.That(value, Is.EqualTo(expectedValue));
        });

        testObject.Dispose();
    }

    [Test]
    public void CastStringObjectTest()
    {
        string expectedValue = "test string";

        //get a StringObject from string as BaseObject
        BaseObject testObject = expectedValue;

        Assert.Multiple(() =>
        {
            Assert.That(testObject, Is.Not.Null);
            Assert.That(testObject.IsDisposed, Is.False);
            Assert.That(testObject, Is.TypeOf<StringObject>());
        });

        //get the string value from a 'StringObject as BaseObject'
        string value = testObject;

        Assert.Multiple(() =>
        {
            Assert.That(value, Is.EqualTo(expectedValue));
        });

        testObject.Dispose();
    }
}
