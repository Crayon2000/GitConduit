package application

type GitLabApp struct {
	config AppConfig
}

func (g *GitLabApp) GetOrganizations() ([]Organization, error) {
	// TODO: implement
	return nil, nil
}

func (g *GitLabApp) GetAuthenticatedUser() (string, error) {
	// TODO: implement
	return "", nil
}

func (g *GitLabApp) GetRepositories(endpoint ApiEndpoint, owner string, authUser string) ([]Repository, error) {
	// TODO: implement
	return nil, nil
}

func (g *GitLabApp) GetIssues(repo Repository) ([]Issue, error) {
	// TODO: implement
	return nil, nil
}

func (g *GitLabApp) CreateRepo(endpoint ApiEndpoint, owner string, source Repository) (Repository, error) {
	// TODO: implement
	return Repository{}, nil
}

func (g *GitLabApp) GetApplicationName() string {
	return "GitLab"
}

func (g *GitLabApp) GetApiUrl() string {
	return g.config.ApiUrl
}

func (g *GitLabApp) SetApiUrl(url string) {
	g.config.ApiUrl = url
}

func (g *GitLabApp) GetToken() string {
	return g.config.Token
}

func (g *GitLabApp) SetToken(token string) {
	g.config.Token = token
}

func (g *GitLabApp) GetUser() string {
	return g.config.User
}

func (g *GitLabApp) SetUser(user string) {
	g.config.User = user
}

func (g *GitLabApp) GetUsername() string {
	return g.config.Username
}

func (g *GitLabApp) SetUsername(username string) {
	g.config.Username = username
}

func (g *GitLabApp) GetPassword() string {
	return g.config.Password
}

func (g *GitLabApp) SetPassword(password string) {
	g.config.Password = password
}
