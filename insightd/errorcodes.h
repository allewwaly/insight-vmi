#ifndef ERRORCODES_H
#define ERRORCODES_H

/// Error codes returned by various functions
enum ErrorCodes {
    ecOk                  = 0,
    ecNoSymbolsLoaded     = -1,
    ecFileNotFound        = -2,
    ecNoFileNameGiven     = -3,
    ecFileNotLoaded       = -4,
    ecInvalidIndex        = -5,
    ecNoMemoryDumpsLoaded = -6,
    ecInvalidArguments    = -7,
    ecCaughtException     = -8,
    ecInvalidId           = -9,
    ecInvalidExpression   = -10
};

#endif // ERRORCODES_H
