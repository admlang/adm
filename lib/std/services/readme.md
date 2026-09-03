A std module describes how programmers interact with a domain. 
A service owns and coordinates the live resources behind that domain.


- Core runtime: Application, Runtime, Policy, Vault, Config
- System resources: Storage, Network, Devices, IPC
- Execution/orchestration: Scheduler, Cache, Messaging
- Observability: Logging, Diagnostics, Debugger, Telemetry
- Application-facing facilities: i18n, UI

- Should have: 
    - Notifications
    - Crypto - centralized secure entropy/key handling:
        secure random
        hashing
        signing
        encryption
        certificate operations 
    - Clipboard
    
- Future: Updater

service Clipboard {
	def get() ?string
	def set(value string)
	def clear()

	def formats() string[]
	def get(format string) ?bytes
	def set(format string, value bytes)

	@emit()
	def changed() {}
}

service Notifications {
	def show(notification Notification) !NotificationId
	def close(id NotificationId) !none

	@emit()
	def action(event NotificationAction) NotificationAction {
		return event
	}

	@emit()
	def closed(id NotificationId) NotificationId {
		return id
	}
}

struct Notification {
	title string
	body ?string
	icon ?string

	level NotificationLevel = NotificationLevel.Info
	actions NotificationActionDefinition[]
	timeout ?duration
}

struct NotificationActionDefinition {
	id string
	label string
}

struct NotificationAction {
	notification NotificationId
	action string
}

notify("Task completed")

notify(
	title = "Connection request",
	message = "192.168.1.42 wants to connect",
	actions = [
		action("allow", "Allow"),
		action("deny", "Deny"),
	]
)

notifications.enabled = true

notifications.security.enabled = true
notifications.tasks.enabled = false
notifications.debug.enabled = false

struct Notification {
	category ?string
	level NotificationLevel
	...
}


Info
Success
Warning
Error
Critical

id = notify(
	title = "Downloading update",
	progress = 0.0
)

notifications.update(id, progress = 0.4)
notifications.update(id, progress = 0.9)
notifications.close(id)

That works nicely for:
    downloads
    builds
    long tasks
    synchronization
    file copies
    background jobs

Not every platform supports every feature, so I think the service should expose capabilities:
notifications.capabilities()

struct NotificationCapabilities {
	actions bool
	progress bool
	images bool
	sound bool
	persistence bool
}

This matters especially with WASM/browser support. Browser notifications, desktop notifications, and perhaps headless environments won't all support identical behavior.

So Policy should probably have an interaction mechanism such as:
policy requests permission
        ↓
if UI available → dialog
if notification actions available → notification
if CLI interactive → terminal prompt
otherwise → deny / configured fallback

Notifications.show(...)
Notifications.update(...)
Notifications.close(...)
Notifications.capabilities()
plus events.