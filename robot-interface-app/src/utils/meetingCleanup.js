import corelink from '../corelink.lib';

/**
 * Cleans up existing meeting resources before rejoining or creating a new meeting
 * Similar to unloadCorelink in Unload.js
 * @param {string} workspace - The current workspace to clean up
 * @returns {Promise<void>}
 */
export const cleanupMeetingBeforeRejoin = async (workspace) => {
  console.log("[MeetingCleanup] Unloading Corelink connections...");
  
  try {
    await corelink.disconnect({ types: 'browser-video' });
    await corelink.disconnect({ types: 'browser-audio' });
    await corelink.disconnect({ types: 'robot-control' });
    await corelink.disconnect({ workspaces: 'Holodeck' });
    if (workspace) {
      await corelink.disconnect({ workspaces: workspace, types: 'robot-control' });
      await corelink.disconnect({ workspaces: workspace, types: 'browser-video' });
      await corelink.disconnect({ workspaces: workspace, types: 'browser-audio' });
    }
  } catch (error) {
    console.error("[MeetingCleanup] Error during disconnect:", error);
  }
}; 
