# HEX SDK

# Test basic native c APIs

cat > network1_0.yml <<EOF
---
# network/network1_0.yml
name: network
version: 1.0

hostname: unconfigured.hex
default-interface: IF.1

dns:
  auto: false
  primary: 1.1.1.1
  secondary: 2.2.2.2
  tertiary: 3.3.3.3

interfaces:
  - enabled: true
    label: IF.1
    speed-duplex: auto
    ipv4:
      enabled: true
      dhcp: false
      ipaddr: 192.168.122.10
      netmask: 255.255.255.0
      gateway: 192.168.122.1
    ipv6:
      enabled: true
      dhcp: true
  - enabled: true
    label: IF.2
    speed-duplex: auto
    ipv4:
      enabled: true
      dhcp: true
    ipv6:
      enabled: true
      dhcp: true
EOF

./$TEST network1_0.yml 2>&1 | tee $TEST.out
