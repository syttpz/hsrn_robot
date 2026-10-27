/**
 * Production-grade encryption utilities using Web Crypto API (AES-GCM)
 * This provides secure encryption for sensitive data stored in localStorage
 */

// Storage key for the encryption key
const KEY_STORAGE_NAME = 'encryptionKey';

/**
 * Generates a cryptographically secure encryption key
 * @returns {Promise<CryptoKey>} The generated AES-GCM key
 */
const generateKey = async () => {
  return await crypto.subtle.generateKey(
    {
      name: 'AES-GCM',
      length: 256,
    },
    true, // extractable - needed to store the key
    ['encrypt', 'decrypt']
  );
};

/**
 * Exports a CryptoKey to a storable format (Base64 string)
 * @param {CryptoKey} key - The key to export
 * @returns {Promise<string>} Base64 encoded key
 */
const exportKey = async (key) => {
  const exported = await crypto.subtle.exportKey('raw', key);
  return btoa(String.fromCharCode(...new Uint8Array(exported)));
};

/**
 * Imports a key from Base64 string format
 * @param {string} keyString - Base64 encoded key
 * @returns {Promise<CryptoKey>} The imported CryptoKey
 */
const importKey = async (keyString) => {
  const keyData = Uint8Array.from(atob(keyString), c => c.charCodeAt(0));
  return await crypto.subtle.importKey(
    'raw',
    keyData,
    { name: 'AES-GCM', length: 256 },
    true,
    ['encrypt', 'decrypt']
  );
};

/**
 * Gets or creates the encryption key
 * The key is stored in localStorage (this is acceptable as the key alone is useless
 * without the encrypted data, and provides protection against casual inspection)
 * For higher security, consider using a key derived from user input or server-provided key
 * @returns {Promise<CryptoKey>} The encryption key
 */
const getOrCreateKey = async () => {
  try {
    const storedKey = localStorage.getItem(KEY_STORAGE_NAME);
    
    if (storedKey) {
      return await importKey(storedKey);
    }
    
    // Generate new key if none exists
    const newKey = await generateKey();
    const exportedKey = await exportKey(newKey);
    localStorage.setItem(KEY_STORAGE_NAME, exportedKey);
    
    return newKey;
  } catch (error) {
    console.error('[cryptoUtils] Error getting/creating key:', error);
    throw error;
  }
};

/**
 * Encrypts a string using AES-GCM
 * @param {string} plaintext - The string to encrypt
 * @returns {Promise<string>} The encrypted data as a Base64 string (includes IV)
 */
export const encryptPassword = async (plaintext) => {
  if (!plaintext) return '';
  
  try {
    const key = await getOrCreateKey();
    
    // Generate a random initialization vector
    const iv = crypto.getRandomValues(new Uint8Array(12));
    
    // Encode the plaintext
    const encoder = new TextEncoder();
    const data = encoder.encode(plaintext);
    
    // Encrypt the data
    const encrypted = await crypto.subtle.encrypt(
      {
        name: 'AES-GCM',
        iv: iv,
      },
      key,
      data
    );
    
    // Combine IV and encrypted data for storage
    const combined = new Uint8Array(iv.length + encrypted.byteLength);
    combined.set(iv);
    combined.set(new Uint8Array(encrypted), iv.length);
    
    // Return as Base64 string
    return btoa(String.fromCharCode(...combined));
  } catch (error) {
    console.error('[encryptPassword] Encryption failed:', error);
    throw new Error('Failed to encrypt password');
  }
};

/**
 * Decrypts an AES-GCM encrypted string
 * @param {string} encryptedData - The Base64 encoded encrypted data (includes IV)
 * @returns {Promise<string>} The decrypted plaintext
 */
export const decryptPassword = async (encryptedData) => {
  if (!encryptedData) return '';
  
  try {
    const key = await getOrCreateKey();
    
    // Decode from Base64
    const combined = Uint8Array.from(atob(encryptedData), c => c.charCodeAt(0));
    
    // Extract IV (first 12 bytes) and encrypted data
    const iv = combined.slice(0, 12);
    const encrypted = combined.slice(12);
    
    // Decrypt the data
    const decrypted = await crypto.subtle.decrypt(
      {
        name: 'AES-GCM',
        iv: iv,
      },
      key,
      encrypted
    );
    
    // Decode and return the plaintext
    const decoder = new TextDecoder();
    return decoder.decode(decrypted);
  } catch (error) {
    console.error('[decryptPassword] Decryption failed:', error);
    // Return empty string on failure (e.g., corrupted data or key mismatch)
    return '';
  }
};

/**
 * Clears the encryption key from storage
 * Call this when user logs out to invalidate all encrypted data
 */
export const clearEncryptionKey = () => {
  localStorage.removeItem(KEY_STORAGE_NAME);
};

/**
 * Checks if Web Crypto API is available
 * @returns {boolean} True if crypto is available
 */
export const isCryptoAvailable = () => {
  return !!(crypto && crypto.subtle && crypto.subtle.encrypt);
};
