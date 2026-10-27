import React, {useState, useRef} from 'react';
import corelink from "../corelink.lib"

const useConnect = ({connection}) => {

    // States
    const [authenticated, setAuthenticated] = useState(false);

    // Refs

    // TODO: IMPLEMENT AUTHENTICATION

    return {
        authenticated
    };
}

export default useConnect;