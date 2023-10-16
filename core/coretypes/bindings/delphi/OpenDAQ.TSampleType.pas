unit OpenDAQ.TSampleType;

interface
type
  {$MINENUMSIZE 4}

  TSampleType = (
    stInvalid = 0,
    stUndefined = 0,
    stFloat32 = 1,
    stFloat64,
    stUInt8,
    stInt8,
    stUInt16,
    stInt16,
    stUInt32,
    stInt32,
    stUInt64,
    stInt64,
    stRangeInt64,
    stComplexFloat32,
    stComplexFloat64,
    stBinary,
    stString
  );

  TScaledSampleType = (
    sstInvalid = 0,
    sstFloat32 = 1,
    sstFloat64
  );

implementation

end.