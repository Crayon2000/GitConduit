package application

type GogsApp struct {
	config AppConfig
}

func (g *GogsApp) GetOrganizations() ([]Organization, error) {
	return GetOrganizations(g.config)
}

func (g *GogsApp) GetAuthenticatedUser() (string, error) {
	return GetAuthenticatedUser(g.config)
}

func (g *GogsApp) GetRepositories(endpoint ApiEndpoint, owner string, authUser string) ([]Repository, error) {
	return GetRepositories(g.config, endpoint, owner, authUser)
}

func (g *GogsApp) GetIssues(repo Repository) ([]Issue, error) {
	return GetIssues(g.config, repo)
}

func (g *GogsApp) CreateRepo(endpoint ApiEndpoint, owner string, source Repository) (Repository, error) {
	return CreateRepo(g.config, endpoint, owner, source)
}

func (g *GogsApp) GetApplicationName() string {
	return "Gogs"
}

func (g *GogsApp) GetApiUrl() string {
	return g.config.ApiUrl
}

func (g *GogsApp) SetApiUrl(url string) {
	g.config.ApiUrl = url
}

func (g *GogsApp) GetToken() string {
	return g.config.Token
}

func (g *GogsApp) SetToken(token string) {
	g.config.Token = token
}

func (g *GogsApp) GetUser() string {
	return g.config.User
}

func (g *GogsApp) SetUser(user string) {
	g.config.User = user
}

func (g *GogsApp) GetUsername() string {
	return g.config.Username
}

func (g *GogsApp) SetUsername(username string) {
	g.config.Username = username
}

func (g *GogsApp) GetPassword() string {
	return g.config.Password
}

func (g *GogsApp) SetPassword(password string) {
	g.config.Password = password
}
