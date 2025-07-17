# PTP Clock Offset Discrepancy Analysis

## Issue Summary

There is a significant discrepancy between the GUI and CLI versions of the PTP tool when measuring system/PHC clock time offset:

- **GUI Output**: System/PHC offset of approximately **-46.5 seconds** (-46,531,877,000 ns)
- **CLI Output**: System/PHC offset of approximately **4.6 milliseconds** (4,660,000 ns)

## Affected Components

- `src/ptptool_gui.cpp` - GUI implementation showing large negative offset
- `src/ptptool_cli.cpp` - CLI implementation showing small positive offset

## Code Analysis

### Timestamp Conversion Function
Both implementations use identical `pctns()` functions:
```cpp
static int64_t pctns(struct ptp_clock_time *t) {
    return t->sec * 1000000000LL + t->nsec;
}
```

### Offset Calculation Logic
Both implementations use identical offset calculation:
```cpp
int64_t t1 = pctns(pct + 2 * i);      // System time before
int64_t tp = pctns(pct + 2 * i + 1);  // PHC time
int64_t t2 = pctns(pct + 2 * i + 2);  // System time after
int64_t offset = (t2 + t1) / 2 - tp;
```

### Device Opening
Both use the same approach:
```cpp
fd = open(getPHCFileName(device).c_str(), O_RDWR);
```

### PTP_SYS_OFFSET Ioctl
Both use identical ioctl calls:
```cpp
ptp_sys_offset sysoff = {};
sysoff.n_samples = samples;
if (ioctl(fd, PTP_SYS_OFFSET, &sysoff))
```

## Observed Data Differences

### GUI Raw Timestamps (Sample 1):
- System time: 1752744017.427858253
- PHC time: 1752744063.959739747 (46+ seconds ahead)
- System time: 1752744017.427866085

### CLI Raw Timestamps:
- System time: 1752744167.360985480  
- PHC time: 1752744167.356330584 (4.6ms behind)

## Potential Root Causes

### 1. Different Device Access
- **Hypothesis**: GUI and CLI might be accessing different PTP devices or the same device in different states
- **Evidence**: The timestamps themselves show completely different base values

### 2. Timestamp Interpretation Issues
- **Hypothesis**: Raw timestamp data from kernel might be corrupted or misinterpreted
- **Evidence**: The 46-second difference is suspiciously large and consistent

### 3. Race Condition or Threading Issues
- **Hypothesis**: GUI runs in multi-threaded environment which might affect timing
- **Evidence**: GUI uses Qt signals/slots and worker threads

### 4. Device State Differences
- **Hypothesis**: The PTP device might be in different synchronization states when accessed by GUI vs CLI
- **Evidence**: The massive time difference suggests one reading might be invalid

### 5. Memory Layout Issues
- **Hypothesis**: Structure padding or memory alignment differences between GUI and CLI builds
- **Evidence**: Both use same headers and structures, but different compilers/flags might affect layout

## Debugging Recommendations

### Immediate Steps:
1. **Add verbose logging** to both CLI and GUI to dump raw `ptp_clock_time` structure values
2. **Check device state** before PTP_SYS_OFFSET calls
3. **Verify same device access** - ensure both are using same `/dev/ptp0`
4. **Add ioctl return value checking** with detailed error reporting

### Investigation Commands:
```bash
# Check available PTP devices
ls -la /dev/ptp*

# Check device capabilities
sudo ./shiwaptptool-cli -d 0 -c

# Run with different sample counts
sudo ./shiwaptptool-cli -d 0 -k 1
sudo ./shiwaptptool-cli -d 0 -k 10
```

### Code Modifications Needed:
1. Add debug prints for raw structure data
2. Add validation of timestamp values before calculation
3. Add device state verification
4. Implement sanity checks for unreasonable offset values

## Priority

**HIGH** - This represents a fundamental measurement error that makes one of the implementations unreliable for PTP synchronization purposes.