# DHCP6C HAL Documentation

## Version History

| Date | Comment | Version |
| --- | --- | --- |
| 2024-07-19 | Initial release. DHCP6C HAL interface header migrated to GitHub under RDKB-52499. | 1.0.0 |
| 2024-09-16 | Maintenance release. The repository-root changelog records the addition of the `dhcp6cInfo_t` structure to `dhcp6cApi.h`. | 1.0.1 |
| 2026-08-24 | Specification rewritten to the canonical RDK-B HAL topic set: both declared APIs named under `API Surface`, the public types and defines documented, the four version identities separated, the observable state values enumerated, and the previously unresolved footprint and timeout requirements replaced with explicit statements. Describes the interface at tag `1.0.1`. | Unreleased |

## Acronyms

- `API` \- Application Programming Interface
- `CPU` \- Central Processing Unit
- `DHCP` \- Dynamic Host Configuration Protocol
- `DHCPv6` \- Dynamic Host Configuration Protocol for IPv6
- `DHCP6C` \- DHCPv6 Client, the subject of this interface
- `DNS` \- Domain Name System
- `ECM` \- Embedded Cable Modem
- `E-Router` \- Edge Router, abbreviated `ERT` in this interface's identifiers
- `HAL` \- Hardware Abstraction Layer
- `IPv6` \- Internet Protocol version 6
- `OEM` \- Original Equipment Manufacturer
- `RDK-B` \- Reference Design Kit for Broadband Devices

## Description

The diagram below describes a high-level software architecture of the DHCP6C HAL module stack.

```mermaid
flowchart TD
    Caller["RDK-B Middleware"] <--> HAL["DHCP6C HAL - dhcp6cApi.h"]
    HAL <--> Vendor["Vendor Software"]
```

The DHCP6C HAL is the `RDK-B` interface for **reading the state of a device's `DHCPv6` clients**. It sits between the RDK-B Middleware and the vendor's `DHCPv6` client implementation, so that a caller obtains the address, prefix, gateway, `DNS` and lease timing a client currently holds without depending on which implementation provides it or which platform it runs on. This repository holds the interface definition only; the implementation is supplied by the vendor, in practice the `OEM` or the silicon supplier.

The interface declares two functions, both reads: one for the `E-Router` `DHCPv6` client and one for the `ECM` `DHCPv6` client. No call configures, starts or stops a client, renews or releases a lease, or subscribes to a change. A caller reads a snapshot; it does not drive the protocol.

**No dedicated `RDK-B` middleware service owns this interface:** The caller, shown as RDK-B Middleware in the diagrams, is any `RDK-B` component that needs `DHCPv6` client state. It links the vendor library directly, so no service has to be stopped before the interface is exercised.

## Optional Components

This interface has **no optional build-time components**. Both functions, both defines and `dhcp6cInfo_t` are declared unconditionally; the include guard `_DHCP6_CLIENT_API_` is the header's only preprocessor conditional.

Optionality exists only as **applicability**, which depends on the product rather than on the build:

- `ert_dhcp6c_get_info` reads the `E-Router` `DHCPv6` client.
- `ecm_dhcp6c_get_info` reads the `ECM` `DHCPv6` client, which exists only on a product with an embedded cable modem. The function is nevertheless declared on every platform.

Neither function's result indicates whether its client is present on the product: a failure is not evidence that the client is absent, and a success is not evidence that it exists.

## Component Runtime Execution Requirements

The requirements in this block govern how an implementation behaves at run time and what a caller may rely on.

### Initialization and Startup

**This interface has no initialization or teardown entry point:** A caller reads values without opening or closing a session. The two functions it may call are:

- `ert_dhcp6c_get_info()` \- retrieves the current state and configuration of the `E-Router` `DHCPv6` client.
- `ecm_dhcp6c_get_info()` \- retrieves the current state and configuration of the `ECM` `DHCPv6` client.

No dependent `API` has to be initialized before either of them is invoked, and no `RDK-B` component has to be started first.

What must be ready is the vendor side. Bringing up and configuring the `DHCPv6` client is the responsibility of the vendor implementation and of whatever component manages network configuration on the product; none of that work is reachable through this interface. Third-party vendors will implement these functions appropriately to meet operational requirements.

### Threading Model

This interface is **not inherently required to be thread-safe**. It is the responsibility of the calling module or component to ensure that all interactions with the APIs are properly synchronized.

**Implementation guidance for vendors:**

Vendors are free to use internal threading or event mechanisms within their implementation as needed to fulfill operational requirements. However, any such mechanisms must:

- **Synchronize access:** Implement appropriate synchronization primitives, such as mutexes or semaphores, to prevent race conditions and ensure data integrity when accessing the APIs.
- **Resource management:** Ensure proper cleanup and release of any resources, such as memory and threads, allocated during the lifetime of the instance, especially during module shutdown or disconnection.

A caller that reads from more than one thread must serialize its own calls and must not share one `dhcp6cInfo_t` between concurrent calls, because two calls filling one instance may interleave their writes.

### Process Model

All APIs are expected to be called from multiple processes. Due to this concurrent access, vendors must implement protection mechanisms within their API implementations to handle multiple processes calling the same API simultaneously. This is crucial to ensure data integrity, prevent race conditions, and maintain the overall stability and reliability of the system.

### Memory Model

The only memory that crosses this interface is the `dhcp6cInfo_t` a caller passes to either function. The interface allocates nothing on the caller's behalf and returns no memory for the caller to free.

#### Caller Responsibilities

- Allocate the `dhcp6cInfo_t` passed to each call, and release it; manage it as outlined in the API documentation to prevent leaks.
- Keep the structure valid for as long as the component continues to call this interface, unless the vendor documents that the implementation does not retain the pointer after a call returns.

#### Module Responsibilities

- Modules must allocate and de-allocate memory for their internal operations, ensuring efficient resource management.
- Modules are required to release all internally allocated memory upon closure to prevent resource leaks.
- All strings a module returns must be zero-terminated, so that string functions can determine their length without overrunning the buffer.

### Power Management Requirements

The HAL is not involved in any power management operation: it neither participates in nor requires involvement in power state transitions, wake event configuration or resource throttling.

### Asynchronous Notification Model

**There are no asynchronous notifications:** A caller that needs to observe a lease being acquired, renewed, rebound or lost **polls** `ert_dhcp6c_get_info()` or `ecm_dhcp6c_get_info()` and compares successive results, typically the state machine position, the configuration attempt count and the lease timers in `dhcp6cInfo_t`.

### Blocking calls

**Synchronous result:** Both functions operate synchronously and deliver their outcome as their return value, so a caller learns it at the call site.

**Caller placement:** A caller does not depend on either call returning promptly. It places each call where a delay is survivable and applies its own timeout where an unresponsive call would be a failure.

### Internal Error Handling

Both functions return their outcome synchronously as an `int`, which distinguishes two outcomes:

| Outcome | Caller action |
| --- | --- |
| Retrieval succeeded | Read the members of `dhcp6cInfo_t`, within the bounds stated under `Data Structures and Defines`. |
| Retrieval failed | Do not read any member of the structure. Re-check the pointer passed, then treat the client's state as unavailable and re-read later rather than substituting a default such as a zero lease. |

Neither outcome has a symbolic name or a numeric value in this interface, so a caller compares the result against its platform's common `RDK-B` status definitions, as agreed with its vendor, and never against a literal. One failure outcome covers every cause, from an invalid argument to an unprepared vendor implementation; the cause is recorded only in the vendor log.

### Persistence Model

There is no requirement for HAL to persist any setting information. The caller is responsible to persist any settings related to their implementation.

Every value a caller reads is owned by the vendor's `DHCPv6` client and retrieved at the moment of the call; this interface stores nothing across a call, a process restart or a reboot.

## Non functional requirements

The following non-functional requirements apply to any implementation of this interface.

### Logging and debugging requirements

The component is required to record all errors and critical informative messages to aid in identifying, debugging, and understanding the functional flow of the system. Logging should be implemented using the `syslog` method, as it provides robust logging capabilities suited for system-level software. The use of `printf` is discouraged unless `syslog` is not available.

All HAL components must adhere to a consistent logging process. When logging is necessary, it should be performed into the `dhcp6c_vendor_hal.log` file, which is located in either the `/var/tmp/` or `/rdklogs/logs/` directories.

Log levels follow the Linux standard logging system, in descending order of severity: `FATAL`, `ERROR`, `WARNING`, `NOTICE`, `INFO`, `DEBUG` and `TRACE`. Each log entry carries a timestamp, the log level and a message describing the event or condition.

On a failed retrieval, an implementation should log which client was requested and what prevented the read, because the return value does not carry the cause.

Implementations must not log credentials, secrets, security-sensitive information, or other sensitive data.

### Memory and performance requirements

**Resource use:** An implementation should not contribute disproportionately to memory or `CPU` utilization during normal operation, and its resource use should be commensurate with the operation requested. Each call returns a **fixed-size structure** with no variable-length or callee-allocated part, so the cost of transferring the result does not grow with the state of the client.

### Quality Control

An implementation should pass analysis with third-party tools such as `Coverity`, `Black Duck` and `Valgrind` without reported issues. Neither the HAL implementation nor the vendor software beneath it may introduce memory leaks or memory corruption.

This HAL is not available on the HUB6 or XER10 reference platforms, so the runtime behaviour this document describes has not been observed on reference hardware.

Any change to a file listed under `Source References` obliges a review of this document by the repository maintainers, through the issue and pull-request process `CONTRIBUTING.md` describes.

### Licensing

The interface definition is licensed under the Apache License 2.0, with the full text in `LICENSE` and the attribution notice in `NOTICE`. An implementation of this interface is expected to be released under the Apache License 2.0.

### Build Requirements

The source code should be capable of, but not be limited to, building under the Yocto distribution environment. The recipe should deliver a shared library named as `libdhcp6cApi.so`.

This repository supplies the interface header only; the library is built by the vendor's recipe.

### Variability Management

Changes to the interface are controlled by versioning. Vendors implement a designated version of the interface and move to later versions as their service agreements require. Each interface version follows [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html), and vendor code complies with a specific version.

This interface has a single variant: no compile-time flag adds or removes any part of it (see `Optional Components`). The interface version is identified by the repository's release tag.

### Platform or Product Customization

This interface has no platform or product customization surface: the same declarations, defines and structure are presented on every platform, and product-specific behavior is confined to the vendor implementation. Which `DHCPv6` clients a product has is a matter of applicability, described under `Optional Components`.

## Interface API Documentation

All HAL function prototypes and datatype definitions are available in the `dhcp6cApi.h` file.

1. Components/Process must include `dhcp6cApi.h` to make use of DHCP6C HAL capabilities.
2. Components/Process should add linker dependency for `libdhcp6cApi.so`.

### Theory of operation and key concepts

The interface has one central concept: a `DHCPv6` client, owned and operated by the vendor implementation, whose current configuration and lease state this interface reports. There are two such clients, one for the `E-Router` and one for the `ECM`, and each has exactly one accessor. Everything this interface does is a read of one of them.

#### Object Lifecycles

- **No explicit object creation:** The DHCP6C HAL interface does not involve explicit creation or destruction of objects within the HAL itself. The primary "object" in this context is the `DHCPv6` client configuration and state information, which is maintained by the underlying vendor implementation.
- **Data retrieval:** The HAL APIs (`ert_dhcp6c_get_info` and `ecm_dhcp6c_get_info`) are designed to retrieve existing `DHCPv6` client information from the vendor implementation and populate a `dhcp6cInfo_t` structure provided by the caller. The lifecycle of this structure is managed by the caller.
- **No handle and no instance:** Neither function takes or returns a handle, a context or an interface identifier; which client is read is fixed by which function is called.

#### Method Sequencing

- **No strict sequencing:** There is no requirement for a specific order in which the methods must be called. However, it is implied that the initialization and configuration of the `DHCPv6` client within the vendor implementation would happen before the HAL APIs are used to retrieve information.
- **Initialization responsibility:** The responsibility for initializing and configuring the `DHCPv6` client lies with the vendor implementation and potentially with higher-level components that manage the network configuration.
- **Independent accessors:** Neither function is a precondition of the other, and calling one has no effect on what the other returns. A caller that reads both treats the two results as taken at two different moments rather than as one consistent snapshot.

#### State-Dependent Behavior

- **Implicit state dependency:** The information returned by `ert_dhcp6c_get_info` and `ecm_dhcp6c_get_info` depends on the current state of the `DHCPv6` client within the vendor implementation.
- **Snapshot values:** Each call returns a snapshot, and the lease timers count down between reads. `State Diagram` lists the state values a caller can observe.

### Data Structures and Defines

The public types and macros below are defined in `dhcp6cApi.h`, where each structure member also carries its own description.

| Definition | Kind | Represents |
| --- | --- | --- |
| `IPV6_ADDR_SIZE` | macro, value `16` | Size of an `IPv6` address in bytes, and the width of every address field in `dhcp6cInfo_t`. |
| `MAX_IPV6_ADDR_LIST_SIZE` | macro, value `3` | Maximum number of `DNS` addresses the structure can carry, and therefore the capacity of `dnsAddrList`. |
| `dhcp6cInfo_t` | struct typedef, 16 members | One `DHCPv6` client's identity, assigned addressing, `DNS` list and lease timing, as reported by the vendor implementation. |

`dhcp6cInfo_t` is the only structure this interface defines. It is allocated by the caller, filled by the implementation, and is the sole out-parameter of both functions. Its members are, in declaration order:

| Member | Type | Meaning |
| --- | --- | --- |
| `ifname` | `char[32]` | Network interface name, zero-terminated within the 32 bytes. |
| `smState` | `int` | Position of the client's state machine. See `State Diagram`. |
| `configAttemps` | `int` | Number of configuration attempts. Spelled as shown in the declaration. |
| `ipAddr` | `unsigned char[16]` | `IPv6` address assigned to the interface. |
| `prefix` | `unsigned char[16]` | Network prefix for the assigned `IPv6` address. |
| `gw` | `unsigned char[16]` | `IPv6` address of the gateway. |
| `dhcpServer` | `unsigned char[16]` | `IPv6` address of the `DHCP` server. |
| `t1` | `unsigned int` | Renewal time (T1) for the lease. |
| `t2` | `unsigned int` | Rebinding time (T2) for the lease. |
| `leaseTime` | `unsigned int` | Total lease time for the address. |
| `leaseGotTime` | `unsigned int` | Timestamp at which the lease was obtained. |
| `remainLeaseTime` | `unsigned int` | Remaining lease time for the address. |
| `remainRenewTime` | `unsigned int` | Remaining time to renewal (T1). |
| `remainRebindTime` | `unsigned int` | Remaining time to rebind (T2). |
| `numOfDns` | `int` | Number of populated entries in `dnsAddrList`. |
| `dnsAddrList` | `unsigned char[3][16]` | List of `DNS` server `IPv6` addresses. |

Three rules decide whether a caller reads the structure correctly:

1. **The address fields are raw bytes, not strings:** `ipAddr`, `prefix`, `gw`, `dhcpServer` and each entry of `dnsAddrList` hold exactly `IPV6_ADDR_SIZE` bytes in wire form. A caller that needs a textual address formats the bytes itself and never passes these fields to a string function.
2. **The count `numOfDns` bounds `dnsAddrList`:** A caller reads entries `0` to `numOfDns - 1` only, and never more than `MAX_IPV6_ADDR_LIST_SIZE` entries.
3. **Timer units come from the vendor:** The interface gives no unit or epoch for the timer members, so a caller confirms them with its vendor before converting any of them.

### API Surface

This interface declares **two** functions; there is no setter and no lifecycle call. Per-API detail, covering argument direction and ownership, pre-conditions, return values and the caller action for each, is in the Doxygen block on each declaration in `dhcp6cApi.h`, within the `DHCPV6C_HAL_APIS` group.

**DHCPv6 client information retrieval (2):**

| API | Purpose |
| --- | --- |
| `ert_dhcp6c_get_info` | Retrieves the current state and configuration of the `E-Router` `DHCPv6` client into a caller-allocated `dhcp6cInfo_t`. |
| `ecm_dhcp6c_get_info` | Retrieves the current state and configuration of the `ECM` `DHCPv6` client into a caller-allocated `dhcp6cInfo_t`. |

Both take one argument, `dhcp6cInfo_t *pInfo`, an out-parameter the caller allocates and owns, and both return `int`.

### Sequence Diagram

The exchange below shows both calls with their real identifiers; either call may be made without the other.

```mermaid
sequenceDiagram
    participant Caller as RDK-B Middleware
    participant HAL as DHCP6C HAL
    participant Vendor as Vendor Software
    Caller->>HAL: ert_dhcp6c_get_info(&info)
    HAL->>Vendor: Read E-Router DHCPv6 client state and lease
    Vendor-->>HAL: Interface name, addresses, DNS list, lease timers
    HAL-->>Caller: Status returned, info populated on success
    Caller->>HAL: ecm_dhcp6c_get_info(&info)
    HAL->>Vendor: Read ECM DHCPv6 client state and lease
    Vendor-->>HAL: Interface name, addresses, DNS list, lease timers
    HAL-->>Caller: Status returned, info populated on success
```

### State Diagram

This interface exposes state as values in `dhcp6cInfo_t` but defines no state transitions, so no state diagram is drawn. A caller polls, as `Asynchronous Notification Model` describes, and interprets these members:

- **Client state machine position:** `smState`. Its values are vendor-defined, so a caller obtains their meaning from its vendor.
- **Configuration progress:** `configAttemps`; a rising value indicates repeated configuration attempts.
- **Lease timing:** `t1` and `t2`, the renewal and rebinding times; `leaseTime`, the total lease duration; `leaseGotTime`, the moment the lease was obtained; and `remainLeaseTime`, `remainRenewTime` and `remainRebindTime`, the time remaining.
- **Assignment:** `ipAddr`, `prefix`, `gw`, `dhcpServer`, `numOfDns` and `dnsAddrList`, the addressing and `DNS` servers the client currently holds.

## Source References

- [rdkcentral/rdkb-halif-dhcp6c/include/dhcp6cApi.h](https://github.com/rdkcentral/rdkb-halif-dhcp6c/blob/develop/include/dhcp6cApi.h) — interface header: function declarations, `dhcp6cInfo_t`, defines and the per-API contract
- [rdkcentral/rdkb-halif-dhcp6c/CHANGELOG.md](https://github.com/rdkcentral/rdkb-halif-dhcp6c/blob/develop/CHANGELOG.md) — release history behind `Version History`
- [rdkcentral/rdkb-halif-dhcp6c/CONTRIBUTING.md](https://github.com/rdkcentral/rdkb-halif-dhcp6c/blob/develop/CONTRIBUTING.md) — contribution and review process named under `Quality Control`
- [rdkcentral/rdkb-halif-dhcp6c/LICENSE](https://github.com/rdkcentral/rdkb-halif-dhcp6c/blob/develop/LICENSE) — Apache License 2.0 text named under `Licensing`
- [rdkcentral/rdkb-halif-dhcp6c/NOTICE](https://github.com/rdkcentral/rdkb-halif-dhcp6c/blob/develop/NOTICE) — attribution notice named under `Licensing`
