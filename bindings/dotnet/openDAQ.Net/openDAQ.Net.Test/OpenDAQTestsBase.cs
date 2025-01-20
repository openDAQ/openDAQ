// Ignore Spelling: Dont

using Daq.Core.Types;

using NUnit.Framework.Interfaces;


namespace openDaq.Net.Test;


/// <summary>
/// Base class for openDAQ .NET Bindings tests which checks for correct object disposal.
/// </summary>
[TestFixture]
public class OpenDAQTestsBase
{
    private ulong _trackedObjectCountOnSetup;
    private bool _doCollectAndFinalize;
    private bool _doCheckAliveObjectCount;
    private bool _doWarn;

    /// <summary>
    /// Set the TearDown function to not to collect and finalize managed objects which would be the default behavior.
    /// </summary>
    protected void DontCollectAndFinalize() => _doCollectAndFinalize = false;

    /// <summary>
    /// Set the TearDown function to not to show the alive object count which would be the default behavior.
    /// </summary>
    protected void DontCheckAliveObjectCount() => _doCheckAliveObjectCount = false;

    /// <summary>
    /// Set the TearDown function to not to assert a warning for alive object handling which would be the default behavior.
    /// </summary>
    protected void DontWarn() => _doWarn = false;

    /// <summary>
    /// Setup the test (prepare).
    /// </summary>
    [SetUp]
    public void BaseSetup()
    {
        _doCollectAndFinalize    = true;
        _doCheckAliveObjectCount = true;
        _doWarn                  = true;

        string testAssemblyName = TestContext.CurrentContext.Test.Type?.Assembly.GetName().Name ?? string.Empty;

        if (testAssemblyName.Contains(".CI.Test") || testAssemblyName.Contains(".NuGet.Test"))
        {
            //turn off everything for any test in CI (standard and NuGet testing)
            DontCollectAndFinalize();
            DontCheckAliveObjectCount();
            DontWarn();
        }

        //cleanup possible remnants from a hard test abortion
        CollectAndFinalize(doLog: false);

        if (CoreTypes.IsTrackingObjects()) //this generally checks if the SDK supports tracking (always true)
        {
            _trackedObjectCountOnSetup = CoreTypes.GetTrackedObjectCount();
        }

        Console.WriteLine($"Executing '{TestContext.CurrentContext.Test.FullName}'");
        Console.WriteLine($"begin of test - {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
        Console.WriteLine("-----------------------------------");
    }

    /// <summary>
    /// Tear down the test (cleanup).
    /// </summary>
    [TearDown]
    public void BaseTearDown()
    {
        //wait just in case cleanup has been done in tests
        GC.WaitForPendingFinalizers();

        Console.WriteLine("-----------------------------------");
        Console.WriteLine($"end of test - {DateTime.Now:yyyy-MM-dd HH:mm:ss}" + Environment.NewLine);

        if (!_doCollectAndFinalize && !_doCheckAliveObjectCount)
        {
            //cleanup possible remnants from a hard test abortion
            CollectAndFinalize(doLog: false);

            return;
        }

        ResultState outcome = TestContext.CurrentContext.Result.Outcome;

        //Label contains "Error" when unhandled Exception occurred; otherwise Empty
        if ((outcome.Status == TestStatus.Failed) && outcome.Label.Equals("Error"))
        {
            Console.WriteLine("*** TearDown skipped due to unhandled exception (no CollectAndFinalize)");
            return; //to log the Exception do not Assert in TearDown
        }

        Console.WriteLine("TearDown --------------------------");

        ulong aliveCount = 0ul;
        if (_doCheckAliveObjectCount
            && CoreTypes.IsTrackingObjects()) //this generally checks if the SDK supports tracking (always true)
        {
            ulong trackedObjectCount = CoreTypes.GetTrackedObjectCount();
            aliveCount = (trackedObjectCount >= _trackedObjectCountOnSetup) ? trackedObjectCount - _trackedObjectCountOnSetup : 0ul;
            Console.WriteLine($"{aliveCount} objects still alive");
        }

        if (_doCollectAndFinalize)
        {
            Console.Write($"Cleanup: ");
            CollectAndFinalize();
        }

        if (_doCheckAliveObjectCount
            && CoreTypes.IsTrackingObjects()) //this generally checks if the SDK supports tracking (always true)
        {
            Console.WriteLine("Checking:");
            ulong trackedObjectCount = CoreTypes.GetTrackedObjectCount();
            aliveCount = (trackedObjectCount >= _trackedObjectCountOnSetup) ? trackedObjectCount - _trackedObjectCountOnSetup : 0ul;

            string message = "OK";

            if (aliveCount > 0)
            {
                message = $"{aliveCount} openDAQ objects are still alive";
                if (_doWarn)
                    Assert.Warn($"*** {message} ***");
            }

            Console.WriteLine("-> " + message);
        }

        if (!_doCollectAndFinalize || (_doCheckAliveObjectCount && CoreTypes.IsTrackingObjects() && (aliveCount > 0)))
        {
            Console.Write(_doCollectAndFinalize ? "   " : "Just in case: ");
            CollectAndFinalize();
        }

        Console.WriteLine("-----------------------------------");
    }

    /// <summary>
    /// Collects the "garbage" and finalizes all those objects (to call <c>BaseObject.Dispose()</c>).
    /// </summary>
    private static void CollectAndFinalize(bool doLog = true)
    {
        //now let the GC clean up (twice, to clean up cycles too)
        //using destructor which calls Dispose()
        if (doLog) Console.WriteLine(">>> collecting garbage and wait for pending finalization <<<");
        try
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
            Thread.Sleep(50);
            GC.Collect();
            GC.WaitForPendingFinalizers();
        }
        catch (Exception ex)
        {
            if (doLog) Console.WriteLine($"*** CollectAndFinalize() threw {ex.GetType().Name} - {ex.Message}");
        }
    }
}
