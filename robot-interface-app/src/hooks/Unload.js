import { useEffect, useCallback } from 'react';
import corelink from "../corelink.lib";

const useUnloadCorelink = (workspace) => {
    const unloadCorelink = useCallback(async () => {
        console.log("Unloading Corelink");
        try {
            await corelink.disconnect({ types: 'browser-video' });
            await corelink.disconnect({ types: 'browser-audio' });
            await corelink.disconnect({ types: 'robot-control' });
            await corelink.disconnect({ workspaces: 'Holodeck' });
            await corelink.disconnect({ workspaces: workspace, types: 'robot-control' });
            await corelink.disconnect({ workspaces: workspace, types: 'browser-video' });
            await corelink.disconnect({ workspaces: workspace, types: 'browser-audio' });
        } catch (error) {
            console.error("Error during disconnect:", error);
        }
    }, [workspace]);

    // Use 'unload' event instead of 'beforeunload' to only cleanup after user confirms leaving
    useEffect(() => {
        window.addEventListener('unload', unloadCorelink);
        return () => {
            window.removeEventListener('unload', unloadCorelink);
        };
    }, [unloadCorelink]);

    return unloadCorelink;
};

export default useUnloadCorelink;