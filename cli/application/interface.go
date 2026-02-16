package application

// Represents an organization.
type Organization struct {
	Name        string
	Description string
}

// Represents a user.
type User struct {
	Login string
}

// Represents a repository.
type Repository struct {
	Owner          User
	Name           string
	FullName       string
	Private        bool
	Description    string
	Fork           bool
	CloneUrl       string
	MirrorUrl      string
	OpenIssueCount int
	HasWiki        bool
	HasIssues      bool
	HasProjects    bool
	HasDownloads   bool
	Homepage       string
}

// Represents an issue.
type Issue struct {
	Title  string
	Body   string
	State  string
	Number int
}

type Application interface {
	GetOrganizations() ([]Organization, error)
	GetAuthenticatedUser() (string, error)
	GetRepositories(endpoint ApiEndpoint, owner string, authUser string) ([]Repository, error)
	GetIssues(repo Repository) ([]Issue, error)
	CreateRepo(endpoint ApiEndpoint, owner string, source Repository) (Repository, error)

	GetApplicationName() string
	GetApiUrl() string
	SetApiUrl(url string)
	GetToken() string
	SetToken(token string)
	GetUser() string
	SetUser(user string)
	GetUsername() string
	SetUsername(username string)
	GetPassword() string
	SetPassword(password string)
}

type ApiEndpoint int

const (
	EndpointUser ApiEndpoint = iota
	EndpointOrganization
)

// Application configuration
type AppConfig struct {
	ApiUrl   string
	Token    string
	User     string
	Username string
	Password string
	Endpoint ApiEndpoint
}

type ApplicationType string

const (
	AppGogs      ApplicationType = "Gogs"
	AppGitBucket ApplicationType = "GitBucket"
	AppGitHub    ApplicationType = "GitHub"
	AppGitLab    ApplicationType = "GitLab"
	AppBitbucket ApplicationType = "Bitbucket"
)

// NewApplication creates a new Application instance based on the given type.
func NewApplication(appType ApplicationType) Application {
	switch appType {
	case AppGogs:
		return &GogsApp{}
	case AppGitBucket:
		return &GitBucketApp{}
	case AppGitHub:
		return &GitHubApp{config: AppConfig{ApiUrl: "https://api.github.com"}}
	case AppGitLab:
		return &GitLabApp{config: AppConfig{ApiUrl: "https://gitlab.com/api/v4"}}
	case AppBitbucket:
		return &BitbucketApp{config: AppConfig{ApiUrl: "https://api.bitbucket.org"}}
	default:
		return nil
	}
}
