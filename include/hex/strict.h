// HEX SDK

#ifndef HEX_STRICT_H
#define HEX_STRICT_H

/**
 *  This file contains the functions that check for STRICT mode.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Check if STRICT mode is enabled.
 *
 *  Checks whether STRICT mode marker file exists.
 *  @return Returns 1 if strict mode is enabled, 0 if STRICT mode is not enabled.
 */
int HexStrictIsEnabled(void);

/** @brief Create the STRICT mode enabled marker.
 *
 *  Creates the STRICT mode marker file.
 */
void HexStrictSetEnabled(void);

/** @brief Check if in STRICT error state.
 *
 *  Checks whether STRICT error marker file exists.
 *  @return Returns 1 if in STRICT error state, 0 if not in STRICT error state.
 */
int HexStrictIsErrorState(void);

/** @brief Create the STRICT error marker.
 *
 *  Creates the STRICT error marker file and forces reboot.
 */
void HexStrictSetErrorState(void);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* endif HEX_STRICT_H */

