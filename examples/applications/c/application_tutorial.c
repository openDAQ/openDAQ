#include <stdio.h>

#include <copendaq.h>

#ifdef _WIN32
#include <windows.h>  // Windows
#else
#include <unistd.h>  // POSIX
#endif

void sleepMs(int milliseconds)
{
#ifdef _WIN32
    Sleep(milliseconds);  // Windows-specific sleep function
#else
    usleep(milliseconds * 1000);  // POSIX-specific sleep function
#endif
}

int main()
{
    // Creating a fresh openDAQ(TM) instance that we will use for all the
    // interactions with the openDAQ(TM) SDK
    daqInstanceBuilder* builder = NULL;
    daqInstanceBuilder_createInstanceBuilder(&builder);

    // Setting the module path to the location where the modules are stored
    daqString* modulePath = NULL;
    daqString_createString(&modulePath,
                           MODULE_PATH);  // Define MODULE_PATH in your CMakeLists.txt
    daqInstanceBuilder_setModulePath(builder, modulePath);
    daqReleaseRef(modulePath);

    daqString* localId = NULL;
    daqString_createString(&localId, "");
    daqInstanceBuilder_setDefaultRootDeviceLocalId(builder, localId);
    daqReleaseRef(localId);

    daqInstance* instance = NULL;
    daqInstance_createInstanceFromBuilder(&instance, builder);
    daqReleaseRef(builder);

    // Borrowing device interface from the instance
    // to get the access to device management functions
    daqDevice* rootDevice = DAQ_BORROW_INTERFACE(instance, DAQ_DEVICE_INTF_ID);

    // Getting the list of available devices
    // and searching for the device with target name
    daqList* availableDevices = NULL;
    daqDevice_getAvailableDevices(rootDevice, &availableDevices);

    daqIterator* iterator = NULL;
    daqList_createStartIterator(availableDevices, &iterator);

    daqDeviceInfo* deviceInfo = NULL;
    while (daqIterator_moveNext(iterator) == DAQ_SUCCESS)
    {
        daqDeviceInfo* currentDevInfo = NULL;
        daqIterator_getCurrent(iterator, (daqBaseObject**) &currentDevInfo);

        if (currentDevInfo == NULL)
        {
            continue;
        }

        daqString* name = NULL;
        daqDeviceInfo_getName(currentDevInfo, &name);

        daqString* targetName = NULL;
        daqString_createString(&targetName, "Reference device simulator");

        daqBaseObject* targetNameObj = DAQ_BORROW_INTERFACE(targetName, DAQ_BASE_OBJECT_INTF_ID);
        daqBaseObject* nameObj = DAQ_BORROW_INTERFACE(name, DAQ_BASE_OBJECT_INTF_ID);

        daqBool equal = False;
        daqBaseObject_equals(nameObj, targetNameObj, &equal);

        daqReleaseRef(name);
        daqReleaseRef(targetName);

        if (equal == True)
        {
            deviceInfo = currentDevInfo;
            break;
        }
        else
        {
            daqReleaseRef(currentDevInfo);
        }
    }

    daqReleaseRef(iterator);
    daqReleaseRef(availableDevices);

    // Exit if no device is found
    if (deviceInfo == NULL)
    {
        printf("Device not found\n");
        daqReleaseRef(instance);
        return 0;
    }

    // Adding the device to the instance
    daqString* connectionString = NULL;
    daqDeviceInfo_getConnectionString(deviceInfo, &connectionString);

    daqDevice* device = NULL;
    daqDevice_addDevice(rootDevice, &device, connectionString, NULL);

    daqReleaseRef(connectionString);
    daqReleaseRef(deviceInfo);

    // Exit if device could not be added
    if (device == NULL)
    {
        printf("Failed to add device\n");
        daqReleaseRef(instance);
        return 0;
    }

    // Getting device information
    daqDevice_getInfo(device, &deviceInfo);

    daqString* name = NULL;
    daqDeviceInfo_getName(deviceInfo, &name);
    daqConstCharPtr nameStr = NULL;
    daqString_getCharPtr(name, &nameStr);

    printf("Device name: %s\n", nameStr);

    daqReleaseRef(name);
    daqReleaseRef(deviceInfo);

    daqSignal* signal = NULL;
    daqFunctionBlock* channel = NULL;  // Channel is a special type of function block

    // Getting the first channel of the device
    daqList* channels = NULL;
    daqDevice_getChannels(device, &channels, NULL);

    daqBaseObject* channelObj = NULL;
    daqList_getItemAt(channels, 0, &channelObj);

    channel = DAQ_QUERY_INTERFACE(channelObj, DAQ_FUNCTION_BLOCK_INTF_ID);

    daqReleaseRef(channels);
    daqReleaseRef(channelObj);

    // Getting the first signal of the channel
    daqList* signals = NULL;
    daqFunctionBlock_getSignals(channel, &signals, NULL);
    daqList_getItemAt(signals, 0, (daqBaseObject**) &signal);

    daqReleaseRef(signals);

    // Getting the last value of the signal
    daqBaseObject* lastValue = NULL;
    daqSignal_getLastValue(signal, &lastValue);

    if (lastValue)
    {
        daqFloatObject* lastValueFloat = NULL;
        lastValueFloat = DAQ_BORROW_INTERFACE(lastValue, DAQ_FLOAT_OBJECT_INTF_ID);
        if (lastValueFloat)
        {
            daqFloat value = 0.0f;
            daqFloatObject_getValue(lastValueFloat, &value);
            printf("Last value: %f\n", value);
        }
        daqReleaseRef(lastValue);
    }

    // Reading samples from signal with stream reader
    daqStreamReader* streamReader = NULL;
    daqStreamReader_createStreamReader(
        &streamReader, signal, daqSampleTypeFloat64, daqSampleTypeInt64, daqReadModeRawValue, daqReadTimeoutTypeAny);

    daqFloat samples[100];  // buffer for reading samples
    for (int i = 0; i < 40; ++i)
    {
        sleepMs(25);
        daqSizeT count = 100;  // would be filled with the number of samples read
        daqStreamReader_read(streamReader, samples, &count, 1000, NULL);
        if (count > 0)
        {
            printf("%6.3f\n", samples[count - 1]);
        }
    }

    // Reading data descriptor
    daqSignal* domainSignal = NULL;
    daqSignal_getDomainSignal(signal, &domainSignal);

    daqDataDescriptor* dataDescriptor = NULL;
    daqSignal_getDescriptor(domainSignal, &dataDescriptor);

    daqRatio* ratio = NULL;
    daqDataDescriptor_getTickResolution(dataDescriptor, &ratio);

    daqString* origin = NULL;
    daqDataDescriptor_getOrigin(dataDescriptor, &origin);

    daqUnit* unit = NULL;
    daqDataDescriptor_getUnit(dataDescriptor, &unit);

    daqString* unitSymbol = NULL;
    daqUnit_getSymbol(unit, &unitSymbol);

    daqConstCharPtr originStr = NULL;
    daqString_getCharPtr(origin, &originStr);

    daqConstCharPtr unitSymbolStr = NULL;
    daqString_getCharPtr(unitSymbol, &unitSymbolStr);

    printf("Origin: %s\n", originStr);

    // Reading domain samples
    daqUInt domainSamples[100];
    for (int i = 0; i < 40; ++i)
    {
        daqSizeT count = 100;
        sleepMs(25);
        daqStreamReader_readWithDomain(streamReader, samples, domainSamples, &count, 1000, NULL);
        if (count > 0)
        {
            // Scaling domain value to the signal unit (seconds)
            daqInt resolutionNumerator = 0;
            daqInt resolutionDenominator = 0;
            daqRatio_getNumerator(ratio, &resolutionNumerator);
            daqRatio_getDenominator(ratio, &resolutionDenominator);

            daqFloat domainValue = (daqFloat) domainSamples[count - 1] * resolutionNumerator / resolutionDenominator;
            printf("Value: %6.3f, Domain: %0.3f%s\n", samples[count - 1], domainValue, unitSymbolStr);
        }
    }

    daqReleaseRef(unitSymbol);
    daqReleaseRef(unit);
    daqReleaseRef(origin);
    daqReleaseRef(ratio);
    daqReleaseRef(dataDescriptor);
    daqReleaseRef(domainSignal);
    daqReleaseRef(streamReader);

    // Adding Renderer function block

    daqString* typeId = NULL;
    daqString_createString(&typeId, "RefFBModuleRenderer");

    daqFunctionBlock* renderer = NULL;
    daqDevice_addFunctionBlock(rootDevice, &renderer, typeId, NULL);

    daqReleaseRef(typeId);

    // Adding Statistics function block
    daqString_createString(&typeId, "RefFBModuleStatistics");

    daqFunctionBlock* statistics = NULL;
    daqDevice_addFunctionBlock(rootDevice, &statistics, typeId, NULL);

    daqReleaseRef(typeId);

    // Connecting the signal to Statistics function block
    daqList* statisticsInputPorts = NULL;
    daqFunctionBlock_getInputPorts(statistics, &statisticsInputPorts, NULL);

    // Getting the first input port of the Statistics function block
    daqInputPort* statisticsInputPort = NULL;
    daqList_getItemAt(statisticsInputPorts, 0, (daqBaseObject**) &statisticsInputPort);

    daqReleaseRef(statisticsInputPorts);

    // Connecting signal to Statistics input port
    daqInputPort_connect(statisticsInputPort, signal);

    daqReleaseRef(statisticsInputPort);

    // Getting statistics output signal
    daqList* statisticsOutputSignals = NULL;
    daqFunctionBlock_getSignals(statistics, &statisticsOutputSignals, NULL);

    daqSignal* statisticsSignal = NULL;
    daqList_getItemAt(statisticsOutputSignals, 0, (daqBaseObject**) &statisticsSignal);

    daqReleaseRef(statisticsOutputSignals);

    // Connecting the signal to Renderer function block
    daqList* rendererInputPorts = NULL;
    daqFunctionBlock_getInputPorts(renderer, &rendererInputPorts, NULL);

    // Getting the first input port of the Renderer function block
    daqInputPort* rendererInputPort = NULL;
    daqList_getItemAt(rendererInputPorts, 0, (daqBaseObject**) &rendererInputPort);

    daqReleaseRef(rendererInputPorts);

    // Connecting signal to Renderer input port
    daqInputPort_connect(rendererInputPort, signal);

    daqReleaseRef(rendererInputPort);

    // Connecting Statistics output signal to Renderer input port
    daqFunctionBlock_getInputPorts(renderer, &rendererInputPorts, NULL);

    // Getting the second input port of the Renderer function block
    daqList_getItemAt(rendererInputPorts, 1, (daqBaseObject**) &rendererInputPort);

    daqReleaseRef(rendererInputPorts);

    // Connecting Statistics output signal to Renderer input port
    daqInputPort_connect(rendererInputPort, statisticsSignal);

    daqReleaseRef(rendererInputPort);
    daqReleaseRef(statisticsSignal);

    // Getting the names of all visible properties of the channel

    // Borrowing the property object interface from the channel
    // to get the access to properties management functions
    daqPropertyObject* channelPropertyObject = DAQ_BORROW_INTERFACE(channel, DAQ_PROPERTY_OBJECT_INTF_ID);

    // Getting the visible properties of the channel
    daqList* properties = NULL;
    daqPropertyObject_getVisibleProperties(channelPropertyObject, &properties);

    daqSizeT propertyCount = 0;
    daqList_getCount(properties, &propertyCount);

    // Printing the names of the properties
    for (daqSizeT i = 0; i < propertyCount; ++i)
    {
        daqProperty* property = NULL;
        daqList_getItemAt(properties, i, (daqBaseObject**) &property);

        daqString* propertyName = NULL;
        daqProperty_getName(property, &propertyName);

        daqConstCharPtr propertyNameStr = NULL;
        daqString_getCharPtr(propertyName, &propertyNameStr);
        printf("%s\n", propertyNameStr);

        daqReleaseRef(propertyName);
        daqReleaseRef(property);
    }

    daqReleaseRef(properties);

    daqString* frequencyPropertyName = NULL;
    daqString_createString(&frequencyPropertyName, "Frequency");

    daqString* noiseAmplitudePropertyName = NULL;
    daqString_createString(&noiseAmplitudePropertyName, "NoiseAmplitude");

    daqString* amplitudePropertyName = NULL;
    daqString_createString(&amplitudePropertyName, "Amplitude");

    daqInteger* frequencyValue = NULL;
    daqInteger_createInteger(&frequencyValue, 5);

    daqFloatObject* noiseAmplitudeValue = NULL;
    daqFloatObject_createFloatObject(&noiseAmplitudeValue, 0.75);

    // Setting the properties of the channel
    daqPropertyObject_setPropertyValue(channelPropertyObject, frequencyPropertyName, (daqBaseObject*) frequencyValue);
    daqPropertyObject_setPropertyValue(channelPropertyObject, noiseAmplitudePropertyName, (daqBaseObject*) noiseAmplitudeValue);

    daqReleaseRef(frequencyValue);
    daqReleaseRef(noiseAmplitudeValue);

    // Modulate the signal amplitude by a step of 0.1 every 25 ms.
    daqFloat ampStep = 0.1;
    for (int i = 0; i < 200; ++i)
    {
        sleepMs(25);

        daqFloatObject* amplitudeValue = NULL;
        daqPropertyObject_getPropertyValue(channelPropertyObject, amplitudePropertyName, (daqBaseObject**) &amplitudeValue);

        daqFloat currentAmplitude = 0.0;
        daqFloatObject_getValue(amplitudeValue, &currentAmplitude);

        if(9.95 < currentAmplitude || currentAmplitude < 1.05)
            ampStep = -ampStep;

        currentAmplitude += ampStep;

        daqReleaseRef(amplitudeValue);

        daqFloatObject_createFloatObject(&amplitudeValue, currentAmplitude);

        daqPropertyObject_setPropertyValue(channelPropertyObject, amplitudePropertyName, (daqBaseObject*) amplitudeValue);

        daqReleaseRef(amplitudeValue);
    }

    printf("Press \"enter\" to exit the application...\n");
    getchar();

    // Releasing all the resources
    // daqReleaseRef returns the reference count of the object after the release
    printf("amplitudePropertyName: %d\n", daqReleaseRef(amplitudePropertyName));
    printf("noiseAmplitudePropertyName: %d\n", daqReleaseRef(noiseAmplitudePropertyName));
    printf("frequencyPropertyName: %d\n", daqReleaseRef(frequencyPropertyName));
    printf("channel: %d\n", daqReleaseRef(channel));
    printf("signal: %d\n", daqReleaseRef(signal));
    printf("device: %d\n", daqReleaseRef(device));
    printf("instance: %d\n", daqReleaseRef(instance));

    return 0;
}