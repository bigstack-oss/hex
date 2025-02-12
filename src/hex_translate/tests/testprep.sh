
DEFAULTS_DIR="/etc/default_policies"
POLICY_DIR="/etc/policies"

rm -rf ${DEFAULTS_DIR}
mkdir -p ${DEFAULTS_DIR}
rm -rf ${POLICY_DIR}
mkdir -p ${POLICY_DIR}
rm -f *.yml