package application

import (
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"strings"
)

type BitbucketApp struct {
	config AppConfig
}

// LinkPayload represents the JSON payload for a link on Bitbucket.
type LinkPayload struct {
	Href string `json:"href"`
	Name string `json:"name"`
}

// ProjectPayload represents the JSON payload for a project on Bitbucket.
type ProjectPayload struct {
	Key string `json:"key,omitempty"`
}

// RepositoryPayload represents the JSON payload for a repository on Bitbucket.
type RepositoryPayload struct {
	Scm         string         `json:"scm"`
	Name        string         `json:"name"`
	FullName    string         `json:"full_name,omitempty"`
	Description string         `json:"description"`
	IsPrivate   bool           `json:"is_private"`
	HasWiki     bool           `json:"has_wiki"`
	HasIssues   bool           `json:"has_issues"`
	Project     ProjectPayload `json:"project"`
	Owner       struct {
		DisplayName string `json:"display_name"`
		UUID        string `json:"uuid"`
	} `json:"owner"`
	Links struct {
		Self  LinkPayload   `json:"self"`
		Clone []LinkPayload `json:"clone"`
	} `json:"links"`
}

// RepositoryListPayload represents the JSON payload for a repository list on Bitbucket.
type RepositoryListPayload struct {
	Page     int                 `json:"page"`
	PageLen  int                 `json:"pagelen"`
	Next     string              `json:"next"`
	Previous string              `json:"previous"`
	Values   []RepositoryPayload `json:"values"`
}

// setBasicAuth sets the Basic Authorization header for a request.
func setBasicAuth(r *http.Request, auth string) {
	r.Header.Set("Authorization", "Basic "+base64.StdEncoding.EncodeToString([]byte(auth)))
}

// Slugify a string.
func slugify(s string) string {
	s = strings.ToLower(s)
	s = strings.ReplaceAll(s, " ", "-")
	var result strings.Builder
	result.Grow(len(s))
	for _, r := range s {
		if (r >= 'a' && r <= 'z') || (r >= '0' && r <= '9') || r == '_' || r == '-' || r == '.' {
			result.WriteRune(r)
		}
	}
	return result.String()
}

func (g *BitbucketApp) GetOrganizations() ([]Organization, error) {
	req, _ := http.NewRequest("GET", g.config.ApiUrl+"/2.0/workspaces", nil)
	setBasicAuth(req, g.config.Token)
	req.Header.Set("Accept", "application/json")
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	if resp.StatusCode == http.StatusNotFound {
		return nil, nil // treat as "no teams"
	}
	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("unexpected status: %d", resp.StatusCode)
	}

	body, _ := io.ReadAll(resp.Body)

	// Bitbucket workspaces response structure
	var response struct {
		Values []struct {
			Slug        string `json:"slug"`
			DisplayName string `json:"name"`
		} `json:"values"`
	}

	if err := json.Unmarshal(body, &response); err != nil {
		return nil, err
	}

	var orgs []Organization
	for _, workspace := range response.Values {
		org := Organization{
			Name:        workspace.Slug,
			Description: workspace.DisplayName,
		}
		orgs = append(orgs, org)
	}

	return orgs, nil
}

func (g *BitbucketApp) GetAuthenticatedUser() (string, error) {
	req, _ := http.NewRequest("GET", g.config.ApiUrl+"/2.0/user", nil)
	setBasicAuth(req, g.config.Token)
	req.Header.Set("Accept", "application/json")
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()
	body, _ := io.ReadAll(resp.Body)

	// Bitbucket user response structure
	var data struct {
		Username string `json:"username"`
		Nickname string `json:"nickname"`
	}
	if err := json.Unmarshal(body, &data); err != nil {
		return "", err
	}

	// Bitbucket uses "username" field instead of "login"
	if data.Username != "" {
		return data.Username, nil
	}
	return data.Nickname, nil
}

func (g *BitbucketApp) GetRepositories(endpoint ApiEndpoint, owner string, authUser string) ([]Repository, error) {
	var url string
	if endpoint == EndpointOrganization {
		// For teams/organizations in Bitbucket
		url = g.config.ApiUrl + "/2.0/repositories/" + owner
	} else {
		// For user repositories
		if owner == authUser {
			url = g.config.ApiUrl + "/2.0/repositories/" + authUser
		} else {
			url = g.config.ApiUrl + "/2.0/repositories/" + owner
		}
	}

	var repos []Repository

	for url != "" {
		req, _ := http.NewRequest("GET", url, nil)
		setBasicAuth(req, g.config.Token)
		req.Header.Set("Accept", "application/json")
		resp, err := http.DefaultClient.Do(req)
		if err != nil {
			return nil, err
		}
		defer resp.Body.Close()

		body, _ := io.ReadAll(resp.Body)

		if resp.StatusCode != http.StatusOK {
			return nil, fmt.Errorf("failed to get repositories: %s", resp.Status)
		}

		// Bitbucket repositories response structure
		var response RepositoryListPayload

		if err := json.Unmarshal(body, &response); err != nil {
			return nil, err
		}

		for _, item := range response.Values {
			repo := Repository{
				Name:        item.Name,
				FullName:    item.FullName,
				Description: item.Description,
				Private:     item.IsPrivate,
				HasWiki:     item.HasWiki,
				HasIssues:   item.HasIssues,
				Owner: User{
					Login: item.Owner.DisplayName,
				},
			}

			// Find HTTPS clone URL
			for _, link := range item.Links.Clone {
				if link.Name == "https" {
					repo.CloneUrl = link.Href
					break
				}
			}

			repos = append(repos, repo)
		}

		url = response.Next // Bitbucket uses "next" field for pagination
	}

	return repos, nil
}

func (g *BitbucketApp) GetIssues(repo Repository) ([]Issue, error) {
	url := g.config.ApiUrl + "/2.0/repositories/" + repo.Owner.Login + "/" + repo.Name + "/issues"
	var issues []Issue

	req, _ := http.NewRequest("GET", url, nil)
	setBasicAuth(req, g.config.Token)
	req.Header.Set("Accept", "application/json")
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	body, _ := io.ReadAll(resp.Body)

	// Bitbucket issues response structure
	var response struct {
		Values []struct {
			ID      int    `json:"id"`
			Title   string `json:"title"`
			Content struct {
				Raw string `json:"raw"`
			} `json:"content"`
			State string `json:"state"`
		} `json:"values"`
	}

	if err := json.Unmarshal(body, &response); err != nil {
		return nil, err
	}

	for _, item := range response.Values {
		issue := Issue{
			Number: item.ID,
			Title:  item.Title,
			Body:   item.Content.Raw,
			State:  item.State,
		}
		issues = append(issues, issue)
	}

	return issues, nil
}

func (g *BitbucketApp) CreateRepo(endpoint ApiEndpoint, owner string, source Repository) (Repository, error) {
	url := g.config.ApiUrl + "/2.0/repositories/" + owner + "/" + slugify(source.Name)

	payload := RepositoryPayload{
		Scm:         "git",
		Name:        source.Name,
		Description: source.Description,
		IsPrivate:   source.Private,
		HasWiki:     source.HasWiki,
		HasIssues:   source.HasIssues,
	}

	projectKey := ""
	if projectKey != "" {
		payload.Project.Key = projectKey
	}

	jsonData, err := json.Marshal(payload)
	if err != nil {
		return Repository{}, err
	}

	req, _ := http.NewRequest("POST", url, strings.NewReader(string(jsonData)))
	setBasicAuth(req, g.config.Token)
	req.Header.Set("Content-Type", "application/json")
	req.Header.Set("Accept", "application/json")
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return Repository{}, err
	}
	defer resp.Body.Close()

	body, _ := io.ReadAll(resp.Body)

	if resp.StatusCode != http.StatusOK {
		return Repository{}, fmt.Errorf("failed to create repo: %s", resp.Status)
	}

	// Parse response
	var response RepositoryPayload

	if err := json.Unmarshal(body, &response); err != nil {
		return Repository{}, err
	}

	repo := Repository{
		Name:        response.Name,
		FullName:    response.FullName,
		Description: response.Description,
		Private:     response.IsPrivate,
		HasWiki:     response.HasWiki,
		HasIssues:   response.HasIssues,
		Owner: User{
			Login: response.Owner.DisplayName,
		},
	}

	// Find HTTPS clone URL
	for _, link := range response.Links.Clone {
		if link.Name == "https" {
			repo.CloneUrl = link.Href
			break
		}
	}

	return repo, nil
}

func (g *BitbucketApp) GetApplicationName() string {
	return "Bitbucket"
}

func (g *BitbucketApp) GetApiUrl() string {
	return g.config.ApiUrl
}

func (g *BitbucketApp) SetApiUrl(url string) {
	g.config.ApiUrl = url
}

func (g *BitbucketApp) GetToken() string {
	return g.config.Token
}

func (g *BitbucketApp) SetToken(token string) {
	g.config.Token = token
}

func (g *BitbucketApp) GetUser() string {
	return g.config.User
}

func (g *BitbucketApp) SetUser(user string) {
	g.config.User = user
}

func (g *BitbucketApp) GetUsername() string {
	return g.config.Username
}

func (g *BitbucketApp) SetUsername(username string) {
	g.config.Username = username
}

func (g *BitbucketApp) GetPassword() string {
	return g.config.Password
}

func (g *BitbucketApp) SetPassword(password string) {
	g.config.Password = password
}

func (g *BitbucketApp) GetEndpoint() ApiEndpoint {
	return g.config.Endpoint
}

func (g *BitbucketApp) SetEndpoint(endpoint ApiEndpoint) {
	g.config.Endpoint = endpoint
}
