 /**
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 /**
 * @file dhcp6cApi.h
 * @brief Interface definition for the RDK-B DHCPv6 Client (DHCP6C) HAL: read-only
 *        access to the state and lease of the E-Router and ECM DHCPv6 clients.
 *
 * This header is the complete DHCP6C HAL contract. It declares the two sizes a caller
 * needs in order to read an address field, one structure carrying a single client's
 * identity, assigned addressing, DNS list and lease timing, and the two functions that
 * fill that structure - one for the E-Router client and one for the ECM (Embedded Cable
 * Modem) client. The clients themselves are created, configured and driven by the
 * vendor implementation; nothing declared here creates, configures or controls one, and
 * there is no setter of any kind.
 *
 * The interface declares no initialization, open, close or teardown call, returns no
 * handle and takes no context, so a caller reads a value on demand and no session is
 * opened or closed. It declares no callback typedef and no registration function, so a
 * caller that needs to observe a lease being acquired, renewed, rebound or lost polls
 * one of the two accessors and compares successive results. The repository
 * specification states both facts, under `Initialization and Startup` and
 * `Asynchronous Notification Model` respectively in the HAL specification.
 *
 * **Return domain.** Both functions return `int` and report their outcome synchronously
 * through that value. This header declares no status enumeration, defines no status
 * macro and includes no other header, and no other file in this repository defines a
 * status name either, so this interface gives its return value neither a symbolic name
 * nor a numeric value. What is and is not established about that value is stated once
 * on the DHCPV6C_HAL_APIS group below and summarised on each declaration; a caller must
 * read it before comparing a result against anything.
 *
 * Behaviour stated in this header is derived from these declarations and from the
 * repository specification, whose topics are cited by name where they are the source.
 * Nothing is presented as observed runtime behaviour, and where neither source
 * establishes a behaviour this header states that it is unspecified rather than filling
 * it in.
 */

#ifndef _DHCP6_CLIENT_API_
#define _DHCP6_CLIENT_API_

#define IPV6_ADDR_SIZE 16           /*!< Width in bytes of every address field of dhcp6cInfo_t - ipAddr, prefix, gw, dhcpServer and each entry of dnsAddrList. An IPv6 address occupies all 16 bytes, so each field is read as exactly this many raw octets and never as text. */
#define MAX_IPV6_ADDR_LIST_SIZE 3   /*!< Element count of the dnsAddrList array of dhcp6cInfo_t. It is the capacity of the DNS list, not the number of entries populated: the populated count is reported separately, in numOfDns. */

/**
 * @brief One DHCPv6 client's identity, assigned addressing, DNS server list and lease
 *        timing, as reported by the vendor implementation.
 *
 * The caller allocates this structure and one of the two accessors fills it in. The
 * interface never allocates it and never frees it, and it is the sole out-parameter of
 * both accessors. Which client it describes is decided by which accessor was called and
 * by nothing inside it: there is no handle, index or interface identifier for a caller
 * to set, and no member selects anything.
 *
 * Three properties decide whether a caller reads it correctly, and none of them is
 * visible from the member names:
 *
 * - The four address members and every entry of dnsAddrList are raw octets - exactly
 *   IPV6_ADDR_SIZE (16) of them, the wire form of an IPv6 address - and not
 *   presentation-format text. A caller that needs a printable address formats the
 *   octets itself, and must not pass any of these members to a string function or print
 *   one as text. Only ifname is a string.
 * - numOfDns bounds dnsAddrList. The array is MAX_IPV6_ADDR_LIST_SIZE (3) entries wide
 *   and numOfDns reports how many of them carry an address, so a caller reads entries 0
 *   to numOfDns - 1 only. This interface does not state what the remaining entries hold,
 *   and does not state that an implementation cannot report a count larger than the
 *   capacity, so a defensive caller clamps the count to MAX_IPV6_ADDR_LIST_SIZE before
 *   indexing.
 * - No member has a documented "absent" value. This interface defines no value meaning
 *   unassigned, unknown or not yet learned for any member, so a caller cannot
 *   distinguish an unpopulated member from a populated one whose octets happen to be
 *   zero, and must treat the whole structure as meaningless unless the call reported
 *   success.
 *
 * @note This interface states no unit for t1, t2, leaseTime, remainLeaseTime,
 *       remainRenewTime and remainRebindTime, and neither a unit nor an epoch for
 *       leaseGotTime. The DHCPv6 protocol expresses T1, T2 and lease lifetimes in
 *       seconds, in RFC 8415, but that is external context: nothing in this interface or
 *       anywhere in this repository binds an implementation to that unit, so a caller
 *       that must convert one of these values confirms the unit with its vendor rather
 *       than assuming it, and a test must not assert one. The repository specification
 *       records the same gap under `Data Structures and Defines`
 *       in the HAL specification.
 * @note Every member is read at the moment of the call and is not preserved by this
 *       interface across a call, a process restart or a reboot. The interface states
 *       neither how old a returned value may be nor how often the vendor client updates
 *       it, so a caller cannot determine from the interface how stale a value is.
 *       `Persistence Model` states that nothing is persisted here.
 * @note One instance must not be shared between concurrent calls. This interface does
 *       not state which members a call writes, or in what order, so two calls filling
 *       one instance may interleave their writes and leave a caller reading a mixture of
 *       two retrievals. `Threading Model` places the obligation to serialise on the
 *       caller.
 *
 * @see ert_dhcp6c_get_info
 * @see ecm_dhcp6c_get_info
 */
typedef struct {
    char ifname[32];                /*!< Name of the network interface the client runs on. The array is 32 bytes and holds the name zero-terminated within those bytes, terminator included, as the `Memory Model` topic of this repository's HAL specification requires of every string this module produces. The 32-byte array is also the bound on what a caller may read from this member. The only string member of this structure: the four address members and every entry of `dnsAddrList` are raw octets. */
    int smState;                    /*!< Position of the vendor client's state machine. This interface defines no value set for it: no enumeration, macro or comment in this repository maps a number to a state, so a caller cannot interpret it without a mapping obtained from its vendor, and a test must not assert a named state. `State Diagram` in the repository specification records the same gap. */
    int configAttemps;              /*!< Number of configuration attempts the client has made. This interface states no bound on it and no threshold at which an implementation stops retrying, so a rising value indicates repeated attempts and nothing more. The member name is spelled as it appears in the declaration. */
    unsigned char ipAddr[IPV6_ADDR_SIZE];     /*!< IPv6 address assigned to the interface, as IPV6_ADDR_SIZE raw octets rather than text. */
    unsigned char prefix[IPV6_ADDR_SIZE];     /*!< Network prefix for the assigned address, as IPV6_ADDR_SIZE raw octets. This structure carries no prefix length, so a caller cannot obtain one from this interface. */
    unsigned char gw[IPV6_ADDR_SIZE];         /*!< IPv6 address of the gateway the client learned, as IPV6_ADDR_SIZE raw octets. */
    unsigned char dhcpServer[IPV6_ADDR_SIZE]; /*!< IPv6 address of the DHCPv6 server that served this client, as IPV6_ADDR_SIZE raw octets. */
    unsigned int t1;                 /*!< Renewal time (T1): the interval after which the client begins renewing the lease with the server that granted it. Unit not stated by this interface. */
    unsigned int t2;                 /*!< Rebinding time (T2): the interval after which the client begins rebinding with any server. Unit not stated by this interface. */
    unsigned int leaseTime;          /*!< Whole lease duration the server granted. It is fixed when the lease is taken and does not count down; remainLeaseTime carries the part still to run. Unit not stated by this interface. */
    unsigned int leaseGotTime;       /*!< Timestamp at which the lease was obtained. Neither the unit nor the epoch it is measured from is stated by this interface, so it is comparable only against another value produced by the same implementation. */
    unsigned int remainLeaseTime;    /*!< Part of leaseTime still to run. It decreases as the lease ages, so two calls a few seconds apart legitimately report different values, and it reaches zero when the lease expires. Unit not stated by this interface. */
    unsigned int remainRenewTime;    /*!< Time left before renewal begins, counting down to the T1 moment. Unit not stated by this interface. */
    unsigned int remainRebindTime;   /*!< Time left before rebinding begins, counting down to the T2 moment. Unit not stated by this interface. */
    int numOfDns;                   /*!< Number of dnsAddrList entries that carry a DNS server address. It bounds what a caller may read - entries 0 to numOfDns - 1 only, never beyond the MAX_IPV6_ADDR_LIST_SIZE capacity. */
    unsigned char dnsAddrList[MAX_IPV6_ADDR_LIST_SIZE][IPV6_ADDR_SIZE]; /*!< DNS server addresses, a fixed MAX_IPV6_ADDR_LIST_SIZE (3) by IPV6_ADDR_SIZE (16) array of raw octets. The populated length is numOfDns; the contents of the entries beyond it are not specified by this interface. */
} dhcp6cInfo_t;

/**
 * @addtogroup DHCPV6C_HAL_APIS
 * @{
 *
 * Both accessors in this group follow one contract, stated here once and summarised on
 * each declaration.
 *
 * **Argument convention.** Each takes exactly one argument, a pointer to a
 * caller-allocated dhcp6cInfo_t, and writes one client's state through it. The caller
 * allocates that storage and releases it; this interface allocates nothing on the
 * caller's behalf and hands out no memory for a caller to free. `Memory Model` in the
 * repository specification places allocation and release on
 * the caller.
 *
 * **How long the storage must live is not settled by this interface.** Neither
 * declaration states whether an implementation retains the pointer after returning, and
 * there is no deregistration call through which a retained pointer could later be
 * released, so this interface does not specify the pointer's lifetime and a caller must
 * not assume either behaviour. The conservative consequence is the caller's: a return is
 * not permission to release the storage, reuse it for something else or let it go out of
 * scope, so a caller either keeps it valid for as long as it goes on using this
 * interface or establishes the retention behaviour with its vendor first. That is the
 * single position this repository takes on retention: `Memory Model` in the repository
 * specification records the same absence in the same terms, and neither it nor these
 * declarations states a non-retention obligation a caller could rely on or test.
 *
 * **Return domain, and what a caller may compare against.** Each accessor returns `int`
 * and reports its outcome synchronously through that value, which is what `Internal
 * Error Handling` requires of every failure - an internal one such as an out-of-memory
 * condition included. Four properties of that value are established by this interface,
 * and nothing beyond them is:
 *
 * - Exactly two outcomes are distinguished: the retrieval succeeded, or it did not.
 * - **Neither outcome has a name in this interface.** This header declares no status
 *   enumeration, defines no status macro and includes no other header, and no other file
 *   in this repository defines a status name either, so there is no name here for a
 *   caller to compile against. Other RDK-B HAL headers do define status macros -
 *   `STATUS_SUCCESS` and `STATUS_FAILURE` among them, each a private definition of the
 *   header that declares it - and none of them is visible through this header, so a
 *   caller must not assume that one of them applies to these two accessors.
 * - **Neither outcome has a numeric value in this interface either.** No success value
 *   and no failure value is stated by this header or by the repository specification, so
 *   a caller must not compare the result against a literal and a test must not assert
 *   one. A caller that has status definitions of its own compares against those, having
 *   established with its vendor that the implementation reports them; a caller that has
 *   none treats the value as opaque, takes the success branch only on the value its
 *   vendor documents, and treats every other value as a failure.
 * - **Failures are not distinguishable.** One failure outcome covers every reason a
 *   retrieval can fail - a rejected pointer, a client that holds no lease yet, an
 *   unprepared vendor implementation, an internal error - and this interface provides no
 *   error code, no error string and no retrieval call through which a caller could learn
 *   which occurred. The vendor log is the only place the reason can be found, which is
 *   why `Logging and debugging requirements` treats logging as load-bearing rather than
 *   optional.
 *
 * **Blocking.** **This interface does not establish whether either accessor may block.**
 * This header states the duration of neither call, states no condition under which one
 * may wait, and says nothing about what either does when the hardware or the DHCPv6
 * client is not yet ready; nothing else in this repository settles the point either, so
 * there is no blocking behaviour for a caller to rely on or for a test to assert. No
 * bound is specified either: neither function takes a timeout argument, no
 * completion-time target or upper bound is stated anywhere here, and there is no
 * cancellation call, no abort and no asynchronous variant, so a call already issued
 * cannot be withdrawn. What a call does deliver is its result, synchronously, through the
 * value it returns - which says where the result appears, not when. The consequences are
 * the caller's: place the call where a delay is survivable, impose and enforce your own
 * bound if the calling context needs one, and do not let a test assert a completion time
 * this interface does not state. `Blocking calls` states this position and no other.
 *
 * **Thread safety.** This interface is not required to be thread safe, and it exposes no
 * lock, handle or context through which an implementation could serialise on a caller's
 * behalf, so the calling module serialises its own calls and must not share one
 * dhcp6cInfo_t between concurrent calls. `Process Model` adds that an implementation must
 * expect calls from several processes and protect its own state accordingly; that
 * obligation is the implementer's and does not relieve a caller of serialising its
 * threads.
 *
 * **Not present.** This group declares no lifecycle call, no setter, no callback
 * registration, no per-interface or per-instance selector and no DHCPv4 accessor. A
 * caller needing any of those is looking at the wrong interface rather than at a gap to
 * be worked around.
 */

/**
 * @brief Reads the current state and lease of the E-Router DHCPv6 client into
 *        caller-supplied storage.
 *
 * A single call reports everything this interface exposes about that client - the
 * interface it runs on, its state machine position and configuration attempt count, the
 * address, prefix, gateway and server it holds, its DNS list and its lease timers - so a
 * caller that needs several of those values takes them from one retrieval rather than
 * calling repeatedly. Because this interface delivers no notification, a caller that must
 * observe a lease being acquired, renewed, rebound or lost calls this function again and
 * compares the result with the previous one; `Asynchronous Notification Model` in the
 * repository specification states that polling is the only
 * mechanism available and that no polling interval is specified.
 *
 * Which client this reads is fixed by the function called, and the only thing this
 * interface says about that client is its name: E-Router, which the declaration comments
 * expand as Edge Router and this interface's identifiers abbreviate as ERT. Beyond the
 * name, **this interface does not define what distinguishes the two clients and does not
 * describe the network role of either**, which `Optional Components` in the repository
 * specification records. A caller must therefore not infer a network role, an address
 * family scope or a physical interface from this function; the interface the
 * implementation associates with the client is reported in the ifname member, and is the
 * only such information available here.
 *
 * @param[out] pInfo Pointer to a caller-allocated dhcp6cInfo_t that the implementation
 *                   fills in. It must be non-NULL and must address writable storage of
 *                   that type. The caller allocates it and releases it, and no ownership
 *                   passes in either direction. This interface does not specify whether
 *                   the implementation retains the pointer beyond the call, so a caller
 *                   must not read the return as permission to release the storage, reuse
 *                   it or let it go out of scope; the argument convention on the
 *                   DHCPV6C_HAL_APIS group above states what a caller does instead. The
 *                   structure's fixed arrays bound what may be read from it: ifname is 32
 *                   bytes including its terminator; ipAddr, prefix, gw and dhcpServer are
 *                   IPV6_ADDR_SIZE (16) raw octets each; and dnsAddrList is
 *                   MAX_IPV6_ADDR_LIST_SIZE (3) entries of IPV6_ADDR_SIZE octets, of
 *                   which only the first numOfDns carry an address. This interface does
 *                   not state whether a null or invalid pointer is detected or
 *                   dereferenced, so a caller must not pass one to find out.
 *
 * @pre The interface declares no initialization, open or teardown call, so there is
 *      nothing for a caller to call first and no session to open or close: the only
 *      pre-condition this interface imposes on a caller is that pInfo addresses writable
 *      storage of the declared type. What must be ready is the vendor side - bringing up
 *      and configuring the DHCPv6 client is the vendor implementation's work, which
 *      `Initialization and Startup` places outside this interface and beyond a caller's
 *      reach. What this interface reports for a call made before that work has completed
 *      is not established, and neither is how long such a call may take: no declaration
 *      here states an outcome for an unready platform, and the blocking note below states
 *      the one position taken on the timing. This function is independent of
 *      ecm_dhcp6c_get_info: neither is a pre-condition of the other, and calling one does
 *      not change what the other reports.
 *
 * @post On success the structure pInfo addresses has been populated and the caller may
 *       read its members, subject to the count and bound rules above. On failure its
 *       content is not specified by this interface - it may be untouched, partially
 *       written or fully written - so a caller must not read any member, and must not
 *       treat a zeroed member as meaning that a value is absent. Nothing inside the
 *       interface changes either way: the call is a read, it stores no setting, and a
 *       failed call leaves nothing for a caller to undo or retry differently.
 *
 * @returns An `int` reporting the outcome of the retrieval synchronously. Exactly two
 *          outcomes are distinguished - the E-Router client's state was retrieved, or it
 *          was not - and this interface gives neither of them a symbolic name and neither
 *          of them a numeric value: this header declares no status enumeration, defines no
 *          status macro and includes no other header, and no other file in this repository
 *          names or values either outcome. A caller therefore resolves the two outcomes
 *          from its own platform status definitions, having established with its vendor
 *          that the implementation reports them, and treats the value as opaque otherwise;
 *          it must not compare the value against a literal, and a test must not assert
 *          one. On the outcome that reports a successful retrieval the post-condition
 *          above applies and the structure may be read. On the outcome that reports a
 *          failure nothing was retrieved: the caller re-checks the pointer it passed,
 *          since a rejected argument is one of the causes that one outcome covers, and
 *          otherwise treats the E-Router client's state as unavailable and re-reads later
 *          rather than substituting a default such as a zero lease. Failures are not
 *          distinguishable from one another - one outcome covers every reason a retrieval
 *          can fail, and this interface offers no error code, error string or retrieval
 *          call for the reason, so the vendor log is the only place it can be found.
 *          Neither outcome is evidence about the client's presence on the product:
 *          `Optional Components` states that this interface establishes neither presence
 *          nor absence.
 *
 * @note This interface does not establish whether this call may block, and it states no
 *       bound on how long it may take - no timeout argument, no cancellation call, no
 *       asynchronous variant. A caller places the call where a delay is survivable and
 *       imposes and enforces its own bound if it needs one, and a test must not assert a
 *       completion time this interface does not state. The group contract above states
 *       this position for both accessors.
 * @note This interface is not required to be thread safe. The calling module serialises
 *       its own calls and must not share one dhcp6cInfo_t between concurrent calls, since
 *       this interface does not state which members a call writes or in what order.
 *       `Threading Model` places that obligation on the caller.
 *
 * @see ecm_dhcp6c_get_info
 * @see dhcp6cInfo_t
 */
int ert_dhcp6c_get_info(dhcp6cInfo_t *pInfo);

/**
 * @brief Reads the current state and lease of the ECM DHCPv6 client into caller-supplied
 *        storage.
 *
 * This function reads the client the declaration identifies as the ECM (Embedded Cable
 * Modem) client, and reads no other; it is otherwise identical to ert_dhcp6c_get_info,
 * with the same argument, the same structure and the same return contract. As there, a
 * single call reports everything this interface exposes about the client, and a caller
 * that must observe a lease change polls and compares successive results because no
 * notification is delivered.
 *
 * Two facts about applicability belong on this declaration in particular, and both come
 * from `Optional Components` in the repository specification
 * in the HAL specification. The client it names exists only on a product that has an
 * embedded cable modem, yet the declaration is unconditional, so this function is
 * presented to callers on every platform whether or not such a client exists. And
 * **beyond the name, this interface neither defines what distinguishes the two clients
 * nor describes the network role of either**, so a caller must not infer a role, an
 * address family scope or a physical interface from this function; the interface the
 * implementation associates with the client is reported in the ifname member.
 *
 * @param[out] pInfo Pointer to a caller-allocated dhcp6cInfo_t that the implementation
 *                   fills in. It must be non-NULL and must address writable storage of
 *                   that type. The caller allocates it and releases it, and no ownership
 *                   passes in either direction. This interface does not specify whether
 *                   the implementation retains the pointer beyond the call, so a caller
 *                   must not read the return as permission to release the storage, reuse
 *                   it or let it go out of scope; the argument convention on the
 *                   DHCPV6C_HAL_APIS group above states what a caller does instead. The
 *                   structure's fixed arrays bound what may be read from it: ifname is 32
 *                   bytes including its terminator; ipAddr, prefix, gw and dhcpServer are
 *                   IPV6_ADDR_SIZE (16) raw octets each; and dnsAddrList is
 *                   MAX_IPV6_ADDR_LIST_SIZE (3) entries of IPV6_ADDR_SIZE octets, of
 *                   which only the first numOfDns carry an address. This interface does
 *                   not state whether a null or invalid pointer is detected or
 *                   dereferenced, so a caller must not pass one to find out.
 *
 * @pre The interface declares no initialization, open or teardown call, so there is
 *      nothing for a caller to call first and no session to open or close: the only
 *      pre-condition this interface imposes on a caller is that pInfo addresses writable
 *      storage of the declared type. What must be ready is the vendor side, as
 *      `Initialization and Startup` records, and what this interface reports for a call
 *      made before that work has completed is not established - as on
 *      ert_dhcp6c_get_info. This function is independent of
 *      ert_dhcp6c_get_info: neither is a pre-condition of the other, calling one does not
 *      change what the other reports, and a caller that reads both must treat the two
 *      results as taken at two different moments rather than as one consistent snapshot.
 *
 * @post On success the structure pInfo addresses has been populated and the caller may
 *       read its members, subject to the count and bound rules above. On failure its
 *       content is not specified by this interface - it may be untouched, partially
 *       written or fully written - so a caller must not read any member, and must not
 *       treat a zeroed member as meaning that a value is absent. Nothing inside the
 *       interface changes either way: the call is a read and stores no setting.
 *
 * @returns An `int` reporting the outcome of the retrieval synchronously. Exactly two
 *          outcomes are distinguished - the ECM client's state was retrieved, or it was
 *          not - and this interface gives neither of them a symbolic name and neither of
 *          them a numeric value: this header declares no status enumeration, defines no
 *          status macro and includes no other header, and no other file in this repository
 *          names or values either outcome. A caller therefore resolves the two outcomes
 *          from its own platform status definitions, having established with its vendor
 *          that the implementation reports them, and treats the value as opaque otherwise;
 *          it must not compare the value against a literal, and a test must not assert
 *          one. On the outcome that reports a successful retrieval the post-condition
 *          above applies and the structure may be read. On the outcome that reports a
 *          failure nothing was retrieved: the caller re-checks the pointer it passed,
 *          since a rejected argument is one of the causes that one outcome covers, and
 *          otherwise treats the ECM client's state as unavailable and re-reads later
 *          rather than substituting a default such as a zero lease. Failures are not
 *          distinguishable from one another - one outcome covers every reason a retrieval
 *          can fail, and this interface offers no error code, error string or retrieval
 *          call for the reason, so the vendor log is the only place it can be found. In
 *          particular the failure outcome does not mean the modem holds no lease, and it
 *          is **not** evidence that the ECM client is absent from the product - which
 *          matters here because this declaration is present on platforms that have no
 *          embedded cable modem. `Optional Components` states that this interface
 *          establishes neither presence nor absence from either outcome.
 *
 * @note This interface does not establish whether this call may block, and it states no
 *       bound on how long it may take - no timeout argument, no cancellation call, no
 *       asynchronous variant. A caller places the call where a delay is survivable and
 *       imposes and enforces its own bound if it needs one, and a test must not assert a
 *       completion time this interface does not state. The group contract above states
 *       this position for both accessors.
 * @note This interface is not required to be thread safe. The calling module serialises
 *       its own calls and must not share one dhcp6cInfo_t between concurrent calls, since
 *       this interface does not state which members a call writes or in what order.
 *       `Threading Model` places that obligation on the caller.
 *
 * @see ert_dhcp6c_get_info
 * @see dhcp6cInfo_t
 */
int ecm_dhcp6c_get_info(dhcp6cInfo_t *pInfo);
/** @} */  //END OF GROUP DHCPV6C_HAL_APIS
#endif
