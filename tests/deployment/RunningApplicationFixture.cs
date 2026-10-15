// TEST ONLY. A real mapped executable image with cooperative, file-driven exit.
// It never loads JobTracker, Qt or application data, and is never shipped.
using System;
using System.IO;
using System.Threading;

internal static class RunningApplicationFixture
{
    private static int Main(string[] args)
    {
        if (args.Length != 2) return 2;
        File.WriteAllText(args[0], "ready");
        DateTime deadline = DateTime.UtcNow.AddMinutes(5);
        while (!File.Exists(args[1]) && DateTime.UtcNow < deadline)
            Thread.Sleep(50);
        return File.Exists(args[1]) ? 0 : 3;
    }
}
