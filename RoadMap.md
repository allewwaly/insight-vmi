# Road Map #

We plan to add new features to InSight in future versions. If you would like to see further enhancements to InSight, feel free to [add a suggestion](https://code.google.com/p/insight-vmi/issues/entry?template=Enhancement) to the issues list.

The following list contains the planned enhancements in the order of their priority:

| **#** | **Feature** | **Description** | **Status** |
|:------|:------------|:----------------|:-----------|
| 1   | C Parser  | Parsing the Linux source code to automatically infer the types of untyped, ambiguous, or "kernel magical" pointers (such as those of `struct list_head` and `struct hlist_node`) | almost done |
| 2   | Windows host support | Porting InSight to Windows (should be fairly easy thanks to Qt libs and little Linux specific code) | not started |
| 3   | Windows guest support | Using the [Microsoft Debugging Tools](http://msdn.microsoft.com/en-us/windows/hardware/gg463009.aspx) to retrieve Windows debugging symbols, parsing these symbols [in PDB format](http://support.microsoft.com/kb/121366/en-us) | not started |