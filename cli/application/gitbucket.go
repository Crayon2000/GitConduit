package application

type GitBucketApp struct {
	config AppConfig
}

func (g *GitBucketApp) GetOrganizations() ([]Organization, error) {
	return GetOrganizations(g.config)
}

func (g *GitBucketApp) GetAuthenticatedUser() (string, error) {
	return GetAuthenticatedUser(g.config)
}

func (g *GitBucketApp) GetRepositories(endpoint ApiEndpoint, owner string, authUser string) ([]Repository, error) {
	return GetRepositories(g.config, endpoint, owner, authUser)
}

func (g *GitBucketApp) GetIssues(repo Repository) ([]Issue, error) {
	return GetIssues(g.config, repo)
}

func (g *GitBucketApp) CreateRepo(endpoint ApiEndpoint, owner string, source Repository) (Repository, error) {
	return CreateRepo(g.config, endpoint, owner, source)
}

func (g *GitBucketApp) GetApplicationName() string {
	return "GitBucket"
}

func (g *GitBucketApp) GetApiUrl() string {
	return g.config.ApiUrl
}

func (g *GitBucketApp) SetApiUrl(url string) {
	g.config.ApiUrl = url
}

func (g *GitBucketApp) GetToken() string {
	return g.config.Token
}

func (g *GitBucketApp) SetToken(token string) {
	g.config.Token = token
}

func (g *GitBucketApp) GetUser() string {
	return g.config.User
}

func (g *GitBucketApp) SetUser(user string) {
	g.config.User = user
}

func (g *GitBucketApp) GetUsername() string {
	return g.config.Username
}

func (g *GitBucketApp) SetUsername(username string) {
	g.config.Username = username
}

func (g *GitBucketApp) GetPassword() string {
	return g.config.Password
}

func (g *GitBucketApp) SetPassword(password string) {
	g.config.Password = password
}
