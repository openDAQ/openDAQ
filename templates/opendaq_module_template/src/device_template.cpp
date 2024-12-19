#include <opendaq_module_template/device_template.h>

BEGIN_NAMESPACE_OPENDAQ_TEMPLATES

void DeviceTemplate::initDevices(const FolderConfigPtr& /*devicesFolder*/)
{
}

void DeviceTemplate::initSyncComponent(const SyncComponentPrivatePtr& /*syncComponent*/)
{
}

void DeviceTemplate::initIOFolder(const IoFolderConfigPtr& /*ioFolder*/)
{
}

void DeviceTemplate::initCustomComponents()
{
}

DeviceDomainPtr DeviceTemplate::initDeviceDomain()
{
    return {};
}

ListPtr<ILogFileInfo> DeviceTemplate::getLogFileInfos()
{
    return {};
}

StringPtr DeviceTemplate::getLog(const StringPtr& /*id*/, Int /*size*/, Int /*offset*/)
{
    return {};
}

uint64_t DeviceTemplate::getTicksSinceOrigin()
{
    return 0;
}

bool DeviceTemplate::allowAddDevicesFromModules()
{
    return false;
}

bool DeviceTemplate::allowAddFunctionBlocksFromModules()
{
    ComponentPtr parent;
    checkErrorInfo(this->componentImpl->getParent(&parent));
    if (!parent.assigned())
        return true;

    return false;
}

AcquisitionLoopParams DeviceTemplate::getAcquisitionLoopParameters()
{
    return {};
}

void DeviceTemplate::onAcquisitionLoop()
{
}

IoFolderConfigPtr DeviceTemplate::createAndAddIOFolder(const std::string& folderId, const IoFolderConfigPtr& parent) const
{
    LOG_T("Creating and adding IO folder with id {}", folderId)
	return componentImpl->addIoFolder(folderId, parent);
}

void DeviceTemplate::setDeviceDomain(const DeviceDomainPtr& deviceDomain) const
{
    LOG_T("Setting device domain")
    componentImpl->setDeviceDomain(deviceDomain);
}

void DeviceTemplate::updateAcquisitionLoop(const AcquisitionLoopParams& params)
{
    LOG_T("Updating acquisition loop")
    componentImpl->updateAcquisitionLoop(params);
}

uint64_t DeviceTemplateHooks::onGetTicksSinceOrigin()
{
    return templateImpl->getTicksSinceOrigin();
}

ListPtr<ILogFileInfo> DeviceTemplateHooks::onGetLogFileInfos()
{
    return templateImpl->getLogFileInfos();
}

StringPtr DeviceTemplateHooks::onGetLog(const StringPtr& id, Int size, Int offset)
{
    return templateImpl->getLog(id, size, offset);
}

bool DeviceTemplateHooks::allowAddDevicesFromModules()
{
    return templateImpl->allowAddDevicesFromModules();
}

bool DeviceTemplateHooks::allowAddFunctionBlocksFromModules()
{
    return templateImpl->allowAddFunctionBlocksFromModules();
}

void DeviceTemplateHooks::removed()
{
    stopAcq = true;
    templateImpl->removed();
    templateImpl.reset();
    GenericDevice::removed();
}

void DeviceTemplateHooks::startLoop()
{
    auto loopLock = std::lock_guard(loopSync);

    if (!acqParams.enableLoop || loopRunning)
        return;
    
    acqThread = std::thread{ &DeviceTemplateHooks::acqLoop, this };
    loopRunning = true;
}

void DeviceTemplateHooks::stopLoop()
{
    auto loopLock = std::lock_guard(loopSync);

    if (!acqParams.enableLoop || !loopRunning)
        return;

    {
        auto lock = this->getRecursiveConfigLock();
        stopAcq = true;
    }

    cv.notify_one();
    acqThread.join();
    loopRunning = false;
}

void DeviceTemplateHooks::acqLoop()
{
    using namespace std::chrono_literals;
    using milli = std::chrono::milliseconds;

    auto startLoopTime = std::chrono::high_resolution_clock::now();
    auto lock = getUniqueLock();

    while (!stopAcq)
    {
        const auto time = std::chrono::high_resolution_clock::now();
        const auto loopDuration = std::chrono::duration_cast<milli>(time - startLoopTime);
        const auto waitTime = loopDuration.count() >= acqParams.loopTime.count() ? milli(0) : milli(acqParams.loopTime.count() - loopDuration.count());
        startLoopTime = time;

        cv.wait_for(lock, waitTime);
        if (!stopAcq)
        {
            templateImpl->onAcquisitionLoop();
        }
    }
}

void DeviceTemplateHooks::updateAcquisitionLoop(const AcquisitionLoopParams& params)
{
    const auto lock = getRecursiveConfigLock();
    acqParams = params;

    if (params.enableLoop)
    {
        startLoop();
    }
    else
    {
        stopLoop();
    }
}

DeviceInfoPtr DeviceTemplateHooks::onGetInfo()
{
    return info;
}

END_NAMESPACE_OPENDAQ_TEMPLATES
