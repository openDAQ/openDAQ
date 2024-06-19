/*
 * Copyright 2022-2024 Blueberry d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CSharpGenerator v1.0.0) on 25.07.2023 14:56.
// </auto-generated>
//
// <modified>DR</modified>
//------------------------------------------------------------------------------

//#define USE_SDK_NOT_MARSHALER_FOR_IUNKNOWN


using System.Reflection;

using Daq.Core.Types.Interfaces;


namespace Daq.Core.Types;


/// <summary>
/// The internally used raw C++ object function-pointers.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe class RawBaseObject : RawIUnknown
{
    //ErrorCode borrowInterface(const daq.core.types.IntfID& intfID, void** obj); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, ref Guid, out IntPtr, ErrorCode> BorrowInterface;
    //ErrorCode dispose(); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, ErrorCode> Dispose;
    //ErrorCode getHashCode(daq.SizeT* hashCode); stdcall;
    public new delegate* unmanaged[Stdcall]<IntPtr, out nuint, ErrorCode> GetHashCode; //new -> hides base member (inherited from Object)
    //ErrorCode equals(daq.IBaseObject* other, daq.Bool* equal); stdcall;
    public new delegate* unmanaged[Stdcall]<IntPtr, IntPtr, out bool, ErrorCode> Equals; //new -> hides base member (inherited from Object)
    //ErrorCode toString(daq.core.types.CharPtr* str); stdcall;
    public new delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> ToString; //new -> hides base member (inherited from Object)
}

/// <summary>
/// The base class for all SDK classes.
/// </summary>
[Guid("9c911f6d-1664-5aa2-97bd-90fe3143e881")]
#pragma warning disable CS0660, CS0661 // Type defines operator == or operator != but does not override Object.Equals(object o) and Object.GetHashCode()
public class BaseObject : IUnknown, IDisposable//, IEquatable<IBaseObject>
#pragma warning restore CS0661, CS0660 // Type defines operator == or operator != but does not override Object.Equals(object o) and Object.Equals(object o)
{
    //wraps this managed object holding a handle to a resource that is passed to unmanaged code using platform invoke.
    private HandleRef _handleRef;

    //holds the function pointers to the C++ class
    internal RawBaseObject _virtualTable;

    //indicates whether this object has already been disposed of
    private bool _disposedValue;

#if DEBUG
    //for debugging purposes only
    private static ulong __instanceCounter = 0ul;
    private readonly string _name;
#endif

    #region Constructors

    internal BaseObject(IntPtr nativePointer, bool incrementReference)
    {
        if (nativePointer == IntPtr.Zero)
        {
            throw new OpenDaqException(ErrorCode.OPENDAQ_ERR_ARGUMENT_NULL, "Error constructing object for native address 0.");
        }

#if DEBUG
        ++__instanceCounter;
        Type type = this.GetType();
        _name = type.Name;
        if (type.IsGenericType)
            _name = _name.Substring(0, _name.IndexOf('`')) + $"<{string.Join(", ", type.GenericTypeArguments.Select(arg => arg.Name).ToArray())}>";
        _name = $"{_name}_{__instanceCounter}";

        System.Diagnostics.Debug.Print("----- Create " + _name);

        //check if the native object implements this object's topmost interface
        CheckNativeInterfaceGuid(nativePointer);
#endif

        // Use a HandleRef to avoid race conditions;
        _handleRef = new HandleRef(this, nativePointer);

        if (incrementReference)
        {
            ((IUnknown)this).AddReference();
        }

        IntPtr objVirtualTable = Marshal.ReadIntPtr(this.NativePointer, 0);     //read the pointer from the given address
        _virtualTable = Marshal.PtrToStructure<RawBaseObject>(objVirtualTable); //use the data from the given pointer address (here function pointers)
    }

    /// <summary>
    /// Creates an instance of the given type using the given parameters.
    /// </summary>
    /// <typeparam name="TObject">The type of the object.</typeparam>
    /// <param name="nativePointer">The native object pointer.</param>
    /// <param name="incrementReference">If set to <c>true</c> increments the reference counter of the native object.</param>
    /// <returns>The instance of the given type.</returns>
    internal static TObject CreateInstance<TObject>(IntPtr nativePointer, bool incrementReference)
        where TObject : BaseObject
    {
        try
        {
            const BindingFlags bindingAttr = BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.CreateInstance; //private constructor

            object obj = Activator.CreateInstance(typeof(TObject), bindingAttr, binder: null, new object[] { nativePointer, incrementReference }, culture: null);
            return (TObject)obj;
        }
        catch (MissingMethodException)
        {
            throw new OpenDaqException(ErrorCode.OPENDAQ_ERR_NOTFOUND, $"Constructor {typeof(TObject).Name}(IntPtr nativePointer, bool incrementReference) not found.");
        }

        ////get the constructor
        //Type objectType = typeof(TObject);
        //var constructor = objectType
        //                  .GetConstructor(BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public, //private constructor
        //                                  binder: null,
        //                                  types: new[] { typeof(IntPtr), typeof(bool) },
        //                                  modifiers: null);

        //if (constructor is null)
        //{
        //    throw new OpenDaqException(ErrorCode.OPENDAQ_ERR_NOTFOUND, $"{objectType.Name}.GetConstructor({{ IntPtr, bool }}) failed.");
        //}

        //return (TObject)constructor.Invoke(new object[] { objectPtr, incrementReference });
    }

    #endregion Constructors

    #region Properties

    internal IntPtr NativePointer
    {
        get
        {
            if (_handleRef.Handle == IntPtr.Zero) //this.IsDisposed not usable here since flag set at the beginning of Dispose()
            {
                throw new OpenDaqException(ErrorCode.OPENDAQ_ERR_INVALID_OPERATION, $"{this.GetType().Name} already disposed or 'stolen' through stealRef");
            }

            return _handleRef.Handle;
        }
    }

    internal HandleRef NativeHandleRef => _handleRef; //ToDo: replace all `base.NativePointer` by `base.NativeHandleRef` for all calls into native code

    #endregion Properties

    #region IUnknown implementation

    /// <summary>Returns another interface which is supported by the object and increments the reference count.</summary>
    /// <remarks>Provides a fundamental mechanism by which an object can express the functionality it provides.</remarks>
    /// <typeparam name="TObject">Object type of requested interface.</typeparam>
    /// <returns>Pointer to the new interface.</returns>
    /// <exception cref="OpenDaqException">Marshal.QueryInterface(TObject.GUID) failed.</exception>
    IntPtr IUnknown.QueryInterface<TObject>()
    {
        Type genericType = typeof(TObject);
        Guid intfID = genericType.GUID;

#if USE_SDK_NOT_MARSHALER_FOR_IUNKNOWN
        //native output argument
        IntPtr objPtr;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_virtualTable.QueryInterface(this.NativePointer, ref intfID, out objPtr);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        return objPtr;
#else
        ErrorCode errorCode = (ErrorCode)Marshal.QueryInterface(this.NativePointer, ref intfID, out IntPtr objPtr); //using RawIUnknown.QueryInterface()

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode, $"Marshal.QueryInterface({genericType.Name}) failed.");
        }

        return objPtr;
#endif
    }

    /// <summary>Increments the reference count for an interface on an object.</summary>
    /// <remarks>You should call this method whenever you make a copy of an interface pointer.</remarks>
    int IUnknown.AddReference()
    {
#if USE_SDK_NOT_MARSHALER_FOR_IUNKNOWN
        unsafe //use native method pointer
        {
            //call native method
            return (int)_virtualTable.AddRef(this.NativePointer);
        }
#else
        int newRefCount = Marshal.AddRef(this.NativePointer); //internally using native RawIUnknown.AddRef()
        return newRefCount;
#endif
    }

    /// <summary>Decrements the reference count for an interface on an object.</summary>
    /// <remarks>Call this method when you no longer need to use an interface pointer.</remarks>
    int IUnknown.ReleaseReference()
    {
        try
        {
#if USE_SDK_NOT_MARSHALER_FOR_IUNKNOWN
            unsafe //use native method pointer
            {
                //call native method
                return (int)_virtualTable.ReleaseRef(this.NativePointer);
            }
#else
            int newRefCount = Marshal.Release(this.NativePointer); //internally using RawIUnknown.Release()
            return newRefCount;
#endif
        }
        catch (Exception ex)
        {
            throw new OpenDaqException(ErrorCode.OPENDAQ_ERR_CALLFAILED, $"IUnknown.ReleaseReference() threw {ex.GetType().Name} - {ex.Message}");
        }
    }

    #endregion IUnknown implementation

    #region operators

    //cast operators to implicitly cast to/from BaseObject from/to .NET value types (but operators to/from derived types not allowed -> use Cast<T>() instead)
    /// <summary>Performs an implicit conversion from <see cref="bool"/> to <see cref="Daq.Core.Types.BaseObject"/>.</summary>
    /// <param name="value">The value to be converted.</param>
    /// <returns>The result of the conversion.</returns>
    public static implicit operator BaseObject(bool value) => CoreTypesFactory.CreateBoolean(value);
    /// <summary>Performs an implicit conversion from <see cref="Daq.Core.Types.BaseObject"/> to <see cref="bool"/>.</summary>
    /// <param name="baseObject">The <see cref="Daq.Core.Types.BaseObject"/> to be converted.</param>
    /// <returns>The result of the conversion.</returns>
    public static implicit operator bool(BaseObject baseObject) { using (BoolObject boolObject = baseObject.Cast<BoolObject>()) return boolObject; }

    /// <summary>Performs an implicit conversion from <see cref="long"/> to <see cref="Daq.Core.Types.BaseObject"/>.</summary>
    /// <param name="value">The value to be converted.</param>
    /// <returns>The result of the conversion.</returns>
    public static implicit operator BaseObject(long value) => CoreTypesFactory.CreateInteger(value);
    /// <summary>Performs an implicit conversion from <see cref="Daq.Core.Types.BaseObject"/> to <see cref="long"/>.</summary>
    /// <param name="baseObject">The <see cref="Daq.Core.Types.BaseObject"/> to be converted.</param>
    /// <returns>The result of the conversion.</returns>
    public static implicit operator long(BaseObject baseObject) { using (IntegerObject integerObject = baseObject.Cast<IntegerObject>()) return integerObject; }

    /// <summary>Performs an implicit conversion from <see cref="double"/> to <see cref="Daq.Core.Types.BaseObject"/>.</summary>
    /// <param name="value">The value to be converted.</param>
    /// <returns>The result of the conversion.</returns>
    public static implicit operator BaseObject(double value) => CoreTypesFactory.CreateFloat(value);
    /// <summary>Performs an implicit conversion from <see cref="Daq.Core.Types.BaseObject"/> to <see cref="double"/>.</summary>
    /// <param name="baseObject">The <see cref="Daq.Core.Types.BaseObject"/> to be converted.</param>
    /// <returns>The result of the conversion.</returns>
    public static implicit operator double(BaseObject baseObject) { using (FloatObject floatObject = baseObject.Cast<FloatObject>()) return floatObject; }

    /// <summary>Performs an implicit conversion from <see cref="string"/> to <see cref="Daq.Core.Types.BaseObject"/>.</summary>
    /// <param name="value">The value to be converted.</param>
    /// <returns>The result of the conversion.</returns>
    public static implicit operator BaseObject(string value) => CoreTypesFactory.CreateString(value);
    /// <summary>Performs an implicit conversion from <see cref="Daq.Core.Types.BaseObject"/> to <see cref="string"/>.</summary>
    /// <param name="baseObject">The <see cref="Daq.Core.Types.BaseObject"/> to be converted.</param>
    /// <returns>The result of the conversion.</returns>
    public static implicit operator string(BaseObject baseObject) { using (StringObject stringObject = baseObject.Cast<StringObject>()) return stringObject; }

    /// <summary>Returns a value that indicates whether two specified SDK objects are equal.</summary>
    /// <remarks>
    ///  For reference objects, it compares the addresses of the objects.<br/>
    ///  For value objects, such as numbers and strings, it compares the values.
    /// </remarks>
    /// <param name="left">The left hand side <c>BaseObject</c>.</param>
    /// <param name="right">The right hand side <c>BaseObject</c>.</param>
    /// <returns><c>true</c> if left and right are equal; otherwise, <c>false</c>.</returns>
    public static bool operator ==(BaseObject left, BaseObject right) => left?.Equals(right) ?? (right is null); //`is null` to avoid recursion

    /// <summary>Returns a value that indicates whether two specified SDK objects are not equal.</summary>
    /// <remarks>
    ///  For reference objects, it compares the addresses of the objects.<br/>
    ///  For value objects, such as numbers and strings, it compares the values.
    /// </remarks>
    /// <param name="left">The left hand side <c>BaseObject</c>.</param>
    /// <param name="right">The right hand side <c>BaseObject</c>.</param>
    /// <returns><c>true</c> if left and right are not equal; otherwise, <c>false</c>.</returns>
    public static bool operator !=(BaseObject left, BaseObject right) => !(left == right);

    #endregion //operators

    /// <summary>Queries the implementation for the existence of the given interface.</summary>
    /// <typeparam name="TObject">The target object type.</typeparam>
    /// <returns>The instance of the type given in <typeparamref name="TObject"/>.</returns>
    public TObject QueryInterface<TObject>()
        where TObject : BaseObject
    {
        IntPtr objPtr = ((IUnknown)this).QueryInterface<TObject>();

        return BaseObject.CreateInstance<TObject>(objPtr, false);
    }

    /// <summary>Returns another interface which is supported by the object without incrementing the reference count.</summary>
    /// <remarks>
    ///  This method is similar to `queryInterface`, however, it does not increment the reference count.
    ///  Use this method if you need to get another interface to the object and the lifetime of the new interface
    ///  is shorter than the lifetime of the original interface.
    /// </remarks>
    /// <returns>The instance of the type given in <typeparamref name="TObject"/>.</returns>
    public TObject BorrowInterface<TObject>()
        where TObject : BaseObject
    {
        Type genericType = typeof(TObject);

        if (genericType == typeof(BaseObject))
        {
            return (TObject)this;
        }

        var intfID = genericType.GUID;

        //native output argument
        IntPtr objPtr;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_virtualTable.BorrowInterface(this.NativePointer, ref intfID, out objPtr);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode, $"BorrowInterface({genericType.Name}) failed.");
            }
        }

        return BaseObject.CreateInstance<TObject>(objPtr, false);
    }

    /// <summary>
    /// Determines whether this instance can be cast to <typeparamref name="TObject"/>.
    /// </summary>
    /// <typeparam name="TObject">The type of the object.</typeparam>
    /// <returns><c>true</c> if this instance can be cast; otherwise, <c>false</c>.</returns>
    public bool CanCastTo<TObject>()
        where TObject : BaseObject
    {
        Type genericType = typeof(TObject);
        if (genericType == typeof(BaseObject))
        {
            return true;
        }

        var intfID = genericType.GUID;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_virtualTable.BorrowInterface(this.NativePointer, ref intfID, out _);

            return Result.Succeeded(errorCode);
        }
    }

    //ToDo: perhaps this should be removed and Query-/BorrowInterface() has to be used directly
    /// <summary>Casts this instance to the possibly derived type <typeparamref name="TObject"/>.</summary>
    /// <remarks>Primary use would be to cast a returned instance of type <see cref="BaseObject"/> or an item of a <see cref="ListObject{BaseObject}"/> to its underlying type.</remarks>
    /// <typeparam name="TObject">The type of the object.</typeparam>
    /// <returns>The cast instance when casting to <typeparamref name="TObject"/> is possible; otherwise <c>null</c>.</returns>
    public TObject Cast<TObject>()
        where TObject : BaseObject
    {
        Type genericType = typeof(TObject);

        if (genericType == typeof(BaseObject))
        {
            return (TObject)this;
        }

        var intfID = genericType.GUID;

        ErrorCode errorCode = (ErrorCode)Marshal.QueryInterface(this.NativePointer, ref intfID, out IntPtr objPtr); //using RawIUnknown.QueryInterface()

        if (Result.Failed(errorCode))
        {
            return null;
        }

        return BaseObject.CreateInstance<TObject>(objPtr, false);
    }

    /// <summary>Casts this instance to <c>ListObject&lt;<typeparamref name="TValue"/>&gt;</c>.</summary>
    /// <typeparam name="TValue">The value type.</typeparam>
    /// <returns>The cast instance when casting to <c>ListObject&lt;<typeparamref name="TValue"/>&gt;</c> is possible; otherwise <c>null</c>.</returns>
    public IListObject<TValue> CastList<TValue>()
        where TValue : BaseObject
    {
        if (this.GetType().Name.StartsWith(nameof(ListObject<BaseObject>)))
            return new ListObject<TValue>(this.NativePointer, true);

        return this.Cast<ListObject<TValue>>();
    }

    /// <summary>Casts this instance to <c>DictObject&lt;<typeparamref name="TKey"/>, <typeparamref name="TValue"/>&gt;</c>.</summary>
    /// <typeparam name="TKey">The key value type.</typeparam>
    /// <typeparam name="TValue">The value type.</typeparam>
    /// <returns>The cast instance when casting to <c>DictObject&lt;<typeparamref name="TKey"/>, <typeparamref name="TValue"/>&gt;</c> is possible; otherwise <c>null</c>.</returns>
    public IDictObject<TKey, TValue> CastDict<TKey, TValue>()
        where TKey : BaseObject
        where TValue : BaseObject
    {
        if (this.GetType().Name.StartsWith(nameof(DictObject<BaseObject, BaseObject>)))
            return new DictObject<TKey, TValue>(this.NativePointer, true);

        return this.Cast<DictObject<TKey, TValue>>();
    }

    /// <summary>
    /// Sets the native pointer to zero, so that the reference count would not be decremented on <see cref="Dispose()"/>. <br/>
    /// Used internally for "stealRef" arguments.
    /// </summary>
    internal void SetNativePointerToZero()
    {
        if (_handleRef.Handle !=  IntPtr.Zero)
            _handleRef = new HandleRef(this, IntPtr.Zero);
    }

    /// <summary>Disposes all references held by the object.</summary>
    /// <remarks>
    ///  An object that holds references to other interfaces must reset these references to null in `dispose`.
    ///  This method is called automatically when the reference count drops to zero.
    /// </remarks>
    private void DisposeObject()
    {
        unsafe //use native method pointer
        {
            //call native method
            try
            {
                ErrorCode errorCode = (ErrorCode)_virtualTable.Dispose(this.NativePointer);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }
            catch (Exception ex)
            {
                throw new OpenDaqException(ErrorCode.OPENDAQ_ERR_INVALID_OPERATION, ex.Message);
            }
        }
    }

    /// <summary>
    /// Checks the native object for implementation of this object's GUID.
    /// </summary>
    /// <param name="nativePointer">The native pointer to check.</param>
    /// <exception cref="OpenDaqException">QueryInterface failed for 'object-type name' with GUID 'xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx'.</exception>
    private void CheckNativeInterfaceGuid(IntPtr nativePointer)
    {
        CheckNativeInterfaceGuid(this.GetType(), nativePointer, doThrowOnError: true);
    }

    internal static ErrorCode CheckNativeInterfaceGuid<TObjectType>(IntPtr nativePointer, bool doThrowOnError = false)
    {
        Type objectType = typeof(TObjectType);

        return CheckNativeInterfaceGuid(objectType, nativePointer, doThrowOnError);
    }

    private static ErrorCode CheckNativeInterfaceGuid(Type objectType, IntPtr nativePointer, bool doThrowOnError)
    {
        //due to its static design this function cannot use BorrowInterface<>() which needs an object instance
        //so we need to cleanup after using QueryInterface() by releasing the acquired pointer

        //interface GUID to check in C++
        Guid guid = objectType.GUID;

        ErrorCode errorCode = (ErrorCode)Marshal.QueryInterface(nativePointer, ref guid, out IntPtr ptr);

        if (Result.Succeeded(errorCode))
        {
            //since QueryInterface() increments the reference counter we need to decrement it again
            // as we only want to check if the interface is available here
            int _ = Marshal.Release(ptr);
        }
        else if (doThrowOnError)
        {
            throw new OpenDaqException(errorCode, $"QueryInterface failed for '{objectType.Name}' with GUID '{guid}'.");
        }

        return errorCode;
    }

    /// <summary>Returns hash code of the object.</summary>
    /// <remarks>
    ///  The object should calculate and return the hash code. For reference objects, it is usually
    ///  calculated from pointer address. For value objects, such as numbers and strings, it is
    ///  calculated from value.
    /// </remarks>
    /// <returns>Hash code.</returns>
    public new nuint GetHashCode()
    {
        //native output argument
        nuint hashCode;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_virtualTable.GetHashCode(this.NativePointer, out hashCode);

            if (Result.Failed(errorCode))
            {
                return (nuint)base.GetHashCode();
            }
        }

        return hashCode;
    }

    /// <summary>
    /// Determines whether the specified object is equal to the current object.
    /// </summary>
    /// <param name="obj">The object to compare with the current object.</param>
    /// <returns><c>true</c> if the specified object is equal to the current object; otherwise, <c>false</c>.</returns>
    public new bool Equals(object obj)
    {
        if (obj is BaseObject baseObject)
        {
            return this.Equals(baseObject);
        }

        return base.Equals(obj);
    }

    /// <summary>Compares object to another object for equality.</summary>
    /// <remarks>
    ///  For reference objects, it compares the address of the object. For value objects, such as numbers and
    ///  strings, it compares values.
    /// </remarks>
    /// <param name="other">Interface to another object for comparison.</param>
    /// <returns>Value indicating if the <c>other</c> is equivalent to this one.</returns>
    public bool Equals(BaseObject other)
    {
        if (other is null) //`is null` to avoid recursion
        {
            return false;
        }

        //native output argument
        bool equal;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_virtualTable.Equals(this.NativePointer, other.NativePointer, out equal);

            if (Result.Failed(errorCode))
            {
                return false;
            }
        }

        return equal;
    }

    /// <summary>Returns a string representation of the object.</summary>
    /// <remarks>Call this method to convert the object to its string representation.</remarks>
    /// <returns>String representation of the object.</returns>
    public override string ToString()
    {
        //native output argument
        IntPtr strPtr;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_virtualTable.ToString(this.NativePointer, out strPtr);

            if (Result.Failed(errorCode))
            {
                return this.GetType().FullName;
            }
        }

        string value = Marshal.PtrToStringAnsi(strPtr);
        return value;
    }

    #region IDisposable implementation

    /// <summary>
    /// Gets a value indicating whether this instance has been disposed of.
    /// </summary>
    public bool IsDisposed => _disposedValue;

    /// <summary>
    /// Disposes resources of this instance.
    /// </summary>
    /// <param name="disposing">If set to <c>true</c> dispose also managed resources; otherwise only native resources will be freed.</param>
    protected virtual void Dispose(bool disposing)
    {
        if (_disposedValue)
        {
            return;
        }

        _disposedValue = true;

        if (disposing)
        {
            // TODO: dispose managed state (managed objects)
        }

        if (_handleRef.Handle == IntPtr.Zero)
        {
            return;
        }

#if DEBUG
        System.Diagnostics.Debug.Write($"----- Dispose({_name}) - ");
#endif
        PrintReferenceCount(debugPrintOnly: true);

        //according to https://learn.microsoft.com/en-us/dotnet/api/system.accessviolationexception?view=net-7.0#accessviolationexception-and-trycatch-blocks
        //Win32 AccessViolationException can be caught by decorating this function with these attributes
        //[HandleProcessCorruptedStateExceptions, SecurityCritical]
        //but the former is marked "obsolete" and not doing anything
        //=> if an object has already been destroyed in native code, there is no way to catch the exception or to find out if it has been destroyed already
        try
        {
            //dispose all references held by the C++ object
            //DisposeObject();

            //release this reference, if it was the last reference the C++ object will be destroyed
            int refCount = ((IUnknown)this).ReleaseReference();

            //System.Diagnostics.Debug.Print("++++> -> reference count after ReleaseReference() = {1}", this.GetType().Name, refCount);
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.Print("+++++> *** {0} Exception - {1}", ex.GetType().Name, ex.Message);
        }
        finally
        {
            //don't permit the handle to be used again
            _handleRef = new HandleRef(this, IntPtr.Zero);
        }
    }

    /// <summary>
    /// Finalizes this instance.
    /// </summary>
    ~BaseObject()
    {
        //make sure that eventually unmanaged resources get freed if not yet done so
        Dispose(disposing: false);
    }

    /// <summary>
    /// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
    /// </summary>
    public void Dispose()
    {
        Dispose(disposing: true);
        GC.SuppressFinalize(this);
    }

    #endregion IDisposable implementation

    /// <summary>In DEBUG configuration it prints the reference count of this object (does nothing otherwise).</summary>
    [System.Diagnostics.Conditional("DEBUG")] //function will never be called in non-DEBUG configuration, even if used in "production" code
    public void PrintReferenceCount(bool debugPrintOnly = false)
    {
#if DEBUG
        if (this.IsDisposed)
        {
            string message = $"'{_name}' already disposed";

            if (!debugPrintOnly)
                Console.WriteLine(message);
            System.Diagnostics.Debug.Print(message);

            return;
        }
        else
        {
            //add and release reference to get the current reference count value
            ((IUnknown)this).AddReference();
            int referenceCount = ((IUnknown)this).ReleaseReference();

            string message = $"{referenceCount} references exist to '{_name}'";

            if (!debugPrintOnly)
                Console.WriteLine(message);
            System.Diagnostics.Debug.Print(message);
        }
#endif
    }
}


#region Class Factory

public static partial class CoreTypesFactory
{
    //ErrorCode createBaseObject(daq.IBaseObject** obj); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createBaseObject(out IntPtr obj);

    /// <summary>
    /// Creates a base object.
    /// </summary>
    /// <param name="obj">Returns the created object.</param>
    /// <returns>Returns <c>ErrorCode.OPENDAQ_SUCCESS</c> if the object could be created, otherwise an <c>ErrorCode</c>.</returns>
    public static ErrorCode CreateBaseObject(out BaseObject obj)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createList(out objPtr);

#if DEBUG
        if (Result.Failed(errorCode))
        {
            return errorCode;
        }

        errorCode = BaseObject.CheckNativeInterfaceGuid<BaseObject>(objPtr);
#endif

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new BaseObject(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    /// <summary>
    /// Creates a base object.
    /// </summary>
    /// <returns>Returns the created object.</returns>
    public static BaseObject CreateBaseObject()
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createBaseObject(out objPtr);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new BaseObject(objPtr, incrementReference: false);
    }
}

#endregion Class Factory
