A std module describes how programmers interact with a domain. 
A service owns and coordinates the live resources behind that domain.

- Core runtime: Application, Runtime, Policy, Vault, Config
- System resources: Storage, Network, Devices, IPC
- Execution/orchestration: Scheduler, Cache, Messaging
- Observability: Logging, Diagnostics, Debugger, Telemetry
- Application-facing facilities: i18n, UI, Notification, Clipboard

- Should have: 
    - Crypto - centralized secure entropy/key handling:
        secure random
        hashing
        signing
        encryption
        certificate operations  
- Future: Updater
