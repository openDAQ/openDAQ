using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Daq.Core.Types;

using NUnit.Framework.Interfaces;


namespace openDaq.Net.Test;


/// <summary>
/// Base class for Daq.Core.Types tests to check for correct object disposal.
/// </summary>
[TestFixture]
public class CoreTypesTestsBase
{
    private ulong _trackedObjectCountOnSetup;
    private bool _doCollectAndFinalize;

    /// <summary>
    /// Set the TearDown function to not to collect and finalize managed objects which would be the default behavior.
    /// </summary>
    protected void DontCollectAndFinalize() => _doCollectAndFinalize = false;

    /// <summary>
    /// Setup the test (prepare).
    /// </summary>
    [SetUp]
    public void BaseSetup()
    {
        //cleanup possible remnants from a hard test abortion
        CollectAndFinalize(doLog: false);

        //set the following line to 'false' when the not finalized object count should be shown
        _doCollectAndFinalize = true;

        if (CoreTypes.IsTrackingObjects()) //this generally checks if the SDK supports tracking (always true)
        {
            _trackedObjectCountOnSetup = CoreTypes.GetTrackedObjectCount();
        }

        Console.WriteLine($"begin of test - {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
        Console.WriteLine("-----------------------------------");
    }

    /// <summary>
    /// Tear down the test (cleanup).
    /// </summary>
    [TearDown]
    public void BaseTearDown()
    {
        Console.WriteLine("-----------------------------------");
        Console.WriteLine($"end of test - {DateTime.Now:yyyy-MM-dd HH:mm:ss}" + Environment.NewLine);

        ResultState outcome = TestContext.CurrentContext.Result.Outcome;

        //Label contains "Error" when unhandled Exception occurred; otherwise Empty
        if ((outcome.Status == TestStatus.Failed) && outcome.Label.Equals("Error"))
        {
            Console.WriteLine("*** TearDown skipped due to unhandled exception (no CollectAndFinalize)");
            return; //to log the Exception do not Assert in TearDown
        }

        Console.WriteLine("TearDown --------------------------");

        ulong aliveCount = 0ul;
        if (CoreTypes.IsTrackingObjects()) //this generally checks if the SDK supports tracking (always true)
        {
            aliveCount = CoreTypes.GetTrackedObjectCount() - _trackedObjectCountOnSetup;
            Console.WriteLine($"{aliveCount} objects still alive");
        }

        if (_doCollectAndFinalize)
        {
            Console.Write($"Cleanup: ");
            CollectAndFinalize();
        }

        if (CoreTypes.IsTrackingObjects()) //this generally checks if the SDK supports tracking (always true)
        {
            Console.WriteLine("Checking:");
            aliveCount = CoreTypes.GetTrackedObjectCount() - _trackedObjectCountOnSetup;

            string message = "OK";

            if (aliveCount > 0)
            {
                message = $"{aliveCount} openDAQ objects are still alive";
                Assert.Warn($"*** {message} ***");
            }

            Console.WriteLine("-> " + message);
        }

        if (!_doCollectAndFinalize || (CoreTypes.IsTrackingObjects() && (aliveCount > 0)))
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
