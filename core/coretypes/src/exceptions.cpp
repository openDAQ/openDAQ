//#include "exceptions.h"
//#include <unordered_map>
//#include <shared_mutex>
//#include <memory>
//
//BEGIN_NAMESPACE_DEWESOFT_RT_CORE
//
//using ErrorCodeToExceptionFactory = std::unordered_map<ErrCode, std::unique_ptr<ExceptionFactory>>;
//
//ErrorCodeToExceptionFactory& errorCodeToException()
//{
//    static ErrorCodeToExceptionFactory map;
//    return map;
//};
//
//extern "C" PUBLIC_EXPORT
//void registerRtException(ErrCode errCode, ExceptionFactory* factory)
//{
//    errorCodeToException()[errCode] = std::unique_ptr<ExceptionFactory>(factory);
//}
//
//extern "C" PUBLIC_EXPORT
//void unregisterException(ErrCode errCode)
//{
//    errorCodeToException().erase(errCode);
//}
//
//extern "C" PUBLIC_EXPORT
//ExceptionFactory* getExceptionFactory(ErrCode errCode)
//{
//    static GenericExceptionFactory<DaqException> defaultFactory;
//    using ConstIter = ErrorCodeToExceptionFactory::const_iterator;
//
//    const ConstIter iterator = errorCodeToException().find(errCode);
//    if (iterator == errorCodeToException().cend())
//    {
//        return &defaultFactory;
//    }
//    return iterator->second.get();
//}
//
//END_NAMESPACE_DEWESOFT_RT_CORE
