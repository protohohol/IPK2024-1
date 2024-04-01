# **Implemented:**
1. TCP and UDP communication according to the provided task.
2. Asynchronous server input and user input handling.
3. OOP.
4. Serialization and deserialization for different types of messages.

# **Known limitations**
1. Message size should be less than 1500 bytes. It doesn't leads to any error, but in TCP variant client doesn't try to continue transmission.
2. Incorrect arguments parsing, in case of `./ipk24chat-client -s -v` for example. It should lead to error, but it doesn't.
3. `ERR: poll` may appear after `C-c` or `C-d`(`EOF`). Do not affect anything (only `stderr` as far as I know). Didn't have enough time to find the problem and fix it. 