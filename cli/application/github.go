package application

type GitHubApp struct {
	config AppConfig
}

func (g *GitHubApp) GetOrganizations() ([]Organization, error) {
	return GetOrganizations(g.config)
}

func (g *GitHubApp) GetAuthenticatedUser() (string, error) {
	return GetAuthenticatedUser(g.config)
}

func (g *GitHubApp) GetRepositories(endpoint ApiEndpoint, owner string, authUser string) ([]Repository, error) {
	return GetRepositories(g.config, endpoint, owner, authUser)
}

func (g *GitHubApp) GetIssues(repo Repository) ([]Issue, error) {
	return GetIssues(g.config, repo)
}

func (g *GitHubApp) CreateRepo(endpoint ApiEndpoint, owner string, source Repository) (Repository, error) {
	return CreateRepo(g.config, endpoint, owner, source)
}

func (g *GitHubApp) GetApplicationName() string {
	return "GitHub"
}

func (g *GitHubApp) GetApiUrl() string {
	return g.config.ApiUrl
}

func (g *GitHubApp) SetApiUrl(url string) {
	g.config.ApiUrl = url
}

func (g *GitHubApp) GetToken() string {
	return g.config.Token
}

func (g *GitHubApp) SetToken(token string) {
	g.config.Token = token
}

func (g *GitHubApp) GetUser() string {
	return g.config.User
}

func (g *GitHubApp) SetUser(user string) {
	g.config.User = user
}

func (g *GitHubApp) GetUsername() string {
	return g.config.Username
}

func (g *GitHubApp) SetUsername(username string) {
	g.config.Username = username
}

func (g *GitHubApp) GetPassword() string {
	return g.config.Password
}

func (g *GitHubApp) SetPassword(password string) {
	g.config.Password = password
}
