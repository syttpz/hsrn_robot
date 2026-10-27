import corelink from '../corelink.lib';
import { encryptPassword, decryptPassword, clearEncryptionKey, isCryptoAvailable } from './cryptoUtils';

/**
 * Attempts to reconnect to corelink using saved credentials from localStorage
 * @returns {Promise<boolean>} Returns true if reconnection was attempted, false if no credentials found
 */
let _reconnectPromise = null;
export const reconnectCorelink = async () => {
  // If a reconnect is already in flight, reuse it to avoid creating duplicate WebSockets
  if (_reconnectPromise) {
    return _reconnectPromise;
  }

  _reconnectPromise = (async () => {
  console.log('[reconnectCorelink] Starting reconnection attempt...');
  
  try {
    const savedCredentials = localStorage.getItem('corelinkCredentials');
    const savedConfigs = localStorage.getItem('corelinkConfigs');
    
    if (!savedCredentials || !savedConfigs) {
      console.log('[reconnectCorelink] No saved credentials found in localStorage');
      return false;
    }
    
    console.log('[reconnectCorelink] Found saved credentials, parsing...');
    const parsedCredentials = JSON.parse(savedCredentials);
    const configs = JSON.parse(savedConfigs);
    
    // Decrypt the password before use
    const decryptedPassword = await decryptPassword(parsedCredentials.password);
    const credentials = {
      ...parsedCredentials,
      password: decryptedPassword
    };
    
    console.log('[reconnectCorelink] Calling corelink.connect() with:', {
      username: credentials.username,
      host: configs.ControlIP,
      port: configs.ControlPort
    });
    
    await corelink.connect(credentials, configs)
      .then(() => {
        console.log('[reconnectCorelink] ✅ Successfully connected to corelink');
      })
      .catch((error) => {
        console.log('[reconnectCorelink] ⚠️ Connection attempt result:', error.message || 'Already connected or connection handled');
      });
    
    console.log('[reconnectCorelink] Reconnection process completed');
    return true;
  } catch (error) {
    console.error('[reconnectCorelink] ❌ Error during reconnection attempt:', error);
    return false;
  } finally {
    _reconnectPromise = null;
  }
  })();

  return _reconnectPromise;
};

/**
 * Saves corelink credentials to localStorage
 * @param {Object} credentials - The credentials object {username, password}
 * @param {Object} configs - The config object {ControlIP, ControlPort}
 * @returns {Promise<boolean>} Returns true if save was successful
 */
export const saveCorelinkCredentials = async (credentials, configs) => {
  try {
    // Encrypt password before saving to localStorage
    const encryptedPassword = await encryptPassword(credentials.password);
    const encryptedCredentials = {
      ...credentials,
      password: encryptedPassword
    };
    
    localStorage.setItem('corelinkCredentials', JSON.stringify(encryptedCredentials));
    localStorage.setItem('corelinkConfigs', JSON.stringify(configs));
    console.log('[saveCorelinkCredentials] ✅ Credentials saved to localStorage (password encrypted)');
    return true;
  } catch (error) {
    console.error('[saveCorelinkCredentials] ❌ Error saving credentials to localStorage:', error);
    return false;
  }
};

/**
 * Retrieves corelink credentials from localStorage
 * @returns {Promise<Object|null>} Returns {credentials, configs} or null if not found
 */
export const getCorelinkCredentials = async () => {
  try {
    const savedCredentials = localStorage.getItem('corelinkCredentials');
    const savedConfigs = localStorage.getItem('corelinkConfigs');
    
    if (savedCredentials && savedConfigs) {
      const parsedCredentials = JSON.parse(savedCredentials);
      // Decrypt password when retrieving
      const decryptedPassword = await decryptPassword(parsedCredentials.password);
      
      return {
        credentials: {
          ...parsedCredentials,
          password: decryptedPassword
        },
        configs: JSON.parse(savedConfigs)
      };
    }
    return null;
  } catch (error) {
    console.error('[getCorelinkCredentials] ❌ Error retrieving credentials from localStorage:', error);
    return null;
  }
};

/**
 * Clears corelink credentials from localStorage
 * Also clears the encryption key to invalidate any stored encrypted data
 */
export const clearCorelinkCredentials = () => {
  try {
    localStorage.removeItem('corelinkCredentials');
    localStorage.removeItem('corelinkConfigs');
    localStorage.removeItem('smSenderID');
    localStorage.removeItem('robotConnection');
    clearEncryptionKey(); // Clear the encryption key as well
    console.log('[clearCorelinkCredentials] ✅ All credentials and session data cleared from localStorage');
  } catch (error) {
    console.error('[clearCorelinkCredentials] ❌ Error clearing credentials:', error);
  }
};

// Re-export crypto utilities for convenience
export { isCryptoAvailable } from './cryptoUtils';
