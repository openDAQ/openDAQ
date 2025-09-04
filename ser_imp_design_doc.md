# Serialization improvements
## Initial state
Current file size for single device:
- 836 kbytes
- 11187 lines
## Proposed changes
### Signal optimizations
Initial state:
49 lines per object
Final state:
6 lines per object
Difference:
-43 lines per object
Impact on file size:
~-424 lines
### Generic component optimizations
Changes:
- Do not serialize statuses
- Do not serialize locked attributes
Difference:
-28 lines per object
Impact on file size:
~-120 lines
###Folder optimizations
Changes:
- Do not serialize folder if empty
Difference:
-3 lines per object

### 1. Store changes only of component configuration

Expected impact on file size:
- N/A
- N/A

Actual impact on file size:
- N/A
- N/A
### 2. Do not serialize DataDescriptors for update

Expected impact on file size:
- N/A
- N/A

Actual impact on file size:
- N/A
- N/A
### 3. Device info optimizations

Do not serialize:
- Server capabilities
- Properties

Expected impact on file size:
- N/A
- N/A

Actual impact on file size:
- N/A
- N/A
### 4. Do not serialize properties of nested property objects

Expected impact on file size:
- N/A
- N/A

Actual impact on file size:
- N/A
- N/A
### 5. Do not serialize defaults

Expected impact on file size:
- N/A
- N/A

Actual impact on file size:
- N/A
- N/A
### 6. Do not serialize redundant intfIDs

Expected impact on file size:
- N/A
- N/A

Actual impact on file size:
- N/A
- N/A
## Final state
Current number of lines:
10643
Current file size:
795 kbytes
Difference in number of lines:
-544 lines
Difference in file size:
-41 kbytes
